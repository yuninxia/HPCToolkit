// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: BSD-3-Clause

// -*-Mode: C++;-*-

//*****************************************************************************
// system includes
//*****************************************************************************

#include <iostream>
#include <chrono>
#include <thread>

//*****************************************************************************
// local includes
//*****************************************************************************

#include "level0-metric-profiler.hpp"
#include "pcsampling-api-receiver.hpp"
#include "pcsampling-api-receiver.hpp"


//******************************************************************************
// static member variables
//******************************************************************************



//******************************************************************************
// type definitions
//******************************************************************************

// Structure to pass arguments to profiling thread
struct profiling_thread_args {
  ZeMetricProfiler* profiler;
  ZeDeviceDescriptor* device;
  const hpcrun_foil_appdispatch_level0* dispatch;
};


//******************************************************************************
// private methods
//******************************************************************************

// Thread wrapper function for hpcrun's thread creation API
static void*
MetricProfilingThreadWrapper(void* arg)
{
  struct profiling_thread_args* args = (struct profiling_thread_args*)arg;

  // Call the actual profiling thread function
  ZeMetricProfiler::MetricProfilingThread(args->profiler, args->device, args->dispatch);

  // Clean up the arguments structure using hpcrun's free
  pcsampling::freeMemory(args);

  return nullptr;
}

static void
FinalizeKernelProcessing
(
  ZeDeviceDescriptor* desc
)
{
  desc->running_kernel_ = nullptr;
  desc->SetKernelStarted(false);
  desc->SetSerialDataReady(true);
}

static bool
WaitForKernelStart
(
  ZeDeviceDescriptor* desc
)
{
  int wait_count = 0;
  const int max_idle_iterations = 10000000; // About 10 seconds of idle waiting

  while (true) {
    // Check if a kernel has been set (this is more reliable than the flag)
    if (desc->running_kernel_ != nullptr) {
      return true;
    }
    if (desc->IsProfilerDisabled()) {
      return false;
    }

    // Timeout mechanism to avoid infinite wait when no more kernels
    if (++wait_count >= max_idle_iterations) {
      // If we've been waiting too long with no kernel activity, assume we're done
      return false;
    }

    std::this_thread::yield();
  }
}

static bool
ProcessMetricData
(
  ZeDeviceDescriptor* desc,
  zet_metric_streamer_handle_t& streamer,
  std::vector<uint8_t>& raw_metrics,
  const std::vector<std::string>& metric_list,
  const std::map<uint64_t, KernelProperties>& kprops,
  const struct hpcrun_foil_appdispatch_level0* dispatch
)
{
  // Define the buffer size
  uint64_t ssize = MAX_METRIC_BUFFER + 512;

  // Read raw metric data from the streamer
  uint64_t raw_size = level0MetricStreamerReadData(streamer, raw_metrics, ssize, dispatch);
  if (raw_size == 0) {
    // No data available yet
    return false;
  }

  // Calculate metric values from the raw data
  std::vector<uint32_t> samples;
  std::vector<zet_typed_value_t> metrics;
  level0MetricGroupCalculateMultipleMetricValuesExp(desc->metric_group_, raw_size, raw_metrics,
                                                  samples, metrics, dispatch);
  if (samples.empty() || metrics.empty()) {
    return false;
  }

  // Process the metric values into stall counts
  std::map<uint64_t, EuStalls> eustalls;
  level0ProcessMetrics(metric_list, samples, metrics, eustalls);
  if (eustalls.empty()) {
    return false;
  }

  // Generate GPU activities based on the processed metrics and kernel properties,
  // then send these activities to the consumer
  std::deque<gpu_activity_t*> activities;
  level0GenerateActivities(kprops, eustalls, desc->correlation_id_, desc->running_kernel_,
                         activities, dispatch);

  level0SendActivities(activities);

  // Activities are passed to hpcrun and will be managed there
  // No need to free them here as ownership has been transferred
  return true;
}

void
ZeMetricProfiler::MetricProfilingThread
(
  ZeMetricProfiler* profiler,
  ZeDeviceDescriptor* desc,
  const struct hpcrun_foil_appdispatch_level0* dispatch
)
{

  ze_context_handle_t context = desc->context_;
  ze_device_handle_t device = desc->device_;
  zet_metric_group_handle_t group = desc->metric_group_;
  zet_metric_streamer_handle_t streamer = nullptr;

  level0InitializeMetricStreamer(context, device, group, streamer, dispatch);

  // Get the list of metrics
  std::vector<std::string> metric_list;
  level0GetMetricList(group, metric_list, dispatch);
  if (!level0IsValidMetricList(metric_list)) {
    // Clean up streamer before returning
    level0CleanupMetricStreamer(context, device, group, streamer, dispatch);
    return;
  }

  RunProfilingLoop(desc, streamer, metric_list, dispatch);

  level0CleanupMetricStreamer(context, device, group, streamer, dispatch);
}

void
ZeMetricProfiler::RunProfilingLoop
(
  ZeDeviceDescriptor* desc,
  zet_metric_streamer_handle_t& streamer,
  std::vector<std::string>& metric_list,
  const struct hpcrun_foil_appdispatch_level0* dispatch
)
{
  std::vector<uint8_t> raw_metrics(MAX_METRIC_BUFFER + 512);
  desc->UpdateProfilerState(PROFILER_ENABLED);
  ze_result_t status;

  // Continue while profiling is enabled
  while (desc->IsProfilerActive()) {

    // Wait for the kernel to start running
    if (!WaitForKernelStart(desc)) {
      return;
    }

    // Update correlation ID
    pcsampling::receiveCorrelationChannel(1, level0UpdateCorrelationId, desc);

    // Continuously collect metrics while the kernel is running
    // Check event status without blocking to see if kernel is still running
    bool kernel_running = true;
    int collection_count = 0;
    while (kernel_running && desc->IsProfilerActive()) {
      // Check if kernel has completed (non-blocking check)
      if (desc->running_kernel_end_ != nullptr) {
        status = pcsampling::callZeEventQueryStatus(desc->running_kernel_end_, dispatch);
        if (status == ZE_RESULT_SUCCESS) {
          kernel_running = false;
        }
      } else {
        // If no event, collect metrics for a fixed duration then assume complete
        kernel_running = false;
      }

      // Collect metrics whether kernel is running or just completed
      CollectAndProcessMetrics(desc, streamer, raw_metrics, metric_list, dispatch);
      collection_count++;

      // Small delay between sampling intervals if kernel is still running
      if (kernel_running) {
        std::this_thread::sleep_for(std::chrono::microseconds(100));
      }
    }


    FinalizeKernelProcessing(desc);
  }

}

void
ZeMetricProfiler::CollectAndProcessMetrics
(
  ZeDeviceDescriptor* desc,
  zet_metric_streamer_handle_t& streamer,
  std::vector<uint8_t>& raw_metrics,
  std::vector<std::string>& metric_list,
  const struct hpcrun_foil_appdispatch_level0* dispatch
)
{
  // Read kernel properties from memory cache
  std::map<uint64_t, KernelProperties> kprops;
  level0ReadKernelProperties(desc->device_id_, kprops);
  if (kprops.empty()) {
    std::cerr << "[WARNING] No kernel properties found for device " << desc->device_id_
              << " - PC sampling data will not be processed" << std::endl;
    return;
  }

  // Continuously process metric data while profiling is enabled
  // This ensures we drain all available data from the streamer buffer
  while (desc->IsProfilerActive()) {
    // FIXME(Yuning): To check the status of the streamer whether it is empty.
    if (!ProcessMetricData(desc, streamer, raw_metrics, metric_list, kprops, dispatch))
      break;  // No more data available
  }
}


//******************************************************************************
// public methods
//******************************************************************************

ZeMetricProfiler*
ZeMetricProfiler::Create
(
  const struct hpcrun_foil_appdispatch_level0* dispatch
)
{
  try {
    ZeMetricProfiler* profiler = new ZeMetricProfiler(dispatch);
    profiler->StartProfilingMetrics(dispatch);
    return profiler;
  } catch (const std::exception& e) {
    fprintf(stderr, "[ERROR] Exception in ZeMetricProfiler::Create: %s\n", e.what());
    std::cerr << "[ERROR] Exception in ZeMetricProfiler::Create: " << e.what() << std::endl;
    return nullptr;
  }
}

ZeMetricProfiler::ZeMetricProfiler
(
  const struct hpcrun_foil_appdispatch_level0* dispatch
)
{
  level0EnumerateDevices(device_descriptors_, metric_contexts_, dispatch);
}

ZeMetricProfiler::~ZeMetricProfiler() 
{
  StopProfilingMetrics();
}

void
ZeMetricProfiler::StartProfilingMetrics
(
  const struct hpcrun_foil_appdispatch_level0* dispatch
)
{
  for (auto it = device_descriptors_.begin(); it != device_descriptors_.end(); ++it) {
    if (it->second->parent_device_ != nullptr) {
      // Skip subdevices
      continue;
    }
    // CRITICAL: Use hpcrun's thread creation API instead of std::thread
    // This ensures proper thread monitoring and lifecycle management

    // Create thread arguments structure
    struct profiling_thread_args* args = (struct profiling_thread_args*)
        pcsampling::allocMemory(sizeof(struct profiling_thread_args));
    if (!args) {
      pcsampling::error("Failed to allocate thread arguments");
      continue;
    }

    args->profiler = this;
    args->device = it->second;
    args->dispatch = dispatch;

    // Create thread using hpcrun's API
    char thread_name[64];
    snprintf(thread_name, sizeof(thread_name), "pcsampling_%p", it->second);

    int thread_id = pcsampling::createProfilingThread(
        MetricProfilingThreadWrapper, args, thread_name);

    if (thread_id < 0) {
      pcsampling::error("Failed to create profiling thread for device %p", it->second);
      pcsampling::freeMemory(args);
      continue;
    }

    it->second->profiling_thread_id_ = thread_id;

    // Wait until profiling is enabled before continuing
    while (!it->second->IsProfilerInitialized()) {
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
  }
}

void
ZeMetricProfiler::StopProfilingMetrics
(
  void
)
{
  for (auto it = device_descriptors_.begin(); it != device_descriptors_.end(); ++it) {
    if (it->second->parent_device_ != nullptr) {
      // Skip subdevices
      continue;
    }

    // Signal the profiling thread to stop
    it->second->UpdateProfilerState(PROFILER_DISABLED);

    // Also clear any kernel started flag to unblock WaitForKernelStart
    it->second->SetKernelStarted(false);

    // Join the profiling thread if it exists
    if (it->second->profiling_thread_id_ >= 0) {
      pcsampling::joinProfilingThread(it->second->profiling_thread_id_);
      it->second->profiling_thread_id_ = -1;
    }
  }

  // Clean up all device descriptors and free memory
  level0CleanupDeviceDescriptors();
  device_descriptors_.clear();
}
