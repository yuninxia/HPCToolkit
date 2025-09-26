// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: BSD-3-Clause

// -*-Mode: C++;-*-

//*****************************************************************************
// system includes
//*****************************************************************************

#include <chrono>
#include <thread>
#include <new>

//*****************************************************************************
// local includes
//*****************************************************************************

#include "level0-metric-profiler.hpp"
#include "level0-pc-api-receiver.hpp"
#include "level0-correlation-channels.h"

#include "../../../../activity/correlation/gpu-channel-common.h"

#include <inttypes.h>


//******************************************************************************
// static member variables
//******************************************************************************



//******************************************************************************
// private methods
//******************************************************************************

namespace {

struct MetricProfilingThreadArgs {
  ZeMetricProfiler* profiler;
  ZeDeviceDescriptor* descriptor;
  const struct hpcrun_foil_appdispatch_level0* dispatch;
};

static void
FinalizeKernelProcessing
(
  ZeDeviceDescriptor* desc
)
{
  desc->running_kernel_ = nullptr;
  desc->running_kernel_end_ = nullptr;
  desc->profiler_ResetKernelState();
}

static bool
WaitForKernelStart
(
  ZeDeviceDescriptor* desc
)
{
  while (true) {
    if (desc->profiler_IsKernelStarted()) return true;
    if (desc->profiler_IsDisabled()) return false;
    std::this_thread::yield();
  }
}

static bool
WaitForNextInterval
(
  ZeDeviceDescriptor* desc,
  const struct hpcrun_foil_appdispatch_level0* dispatch,
  ze_result_t& status
)
{
  // If no signal event was provided, consider kernel as immediately finished
  if (desc->running_kernel_end_ == nullptr) {
    status = ZE_RESULT_SUCCESS;
    return true;
  }

  while (true) {
    status = pcsampling::callZeEventQueryStatus(desc->running_kernel_end_, dispatch);
    if (status == ZE_RESULT_SUCCESS) return true;
    if (desc->profiler_IsDisabled()) return false;
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
  if (raw_size == 0) return false;

  // Calculate metric values from the raw data
  std::vector<uint32_t> samples;
  std::vector<zet_typed_value_t> metrics;
  level0MetricGroupCalculateMultipleMetricValuesExp(desc->metric_group_, raw_size, raw_metrics,
                                                  samples, metrics, dispatch);
  if (samples.empty() || metrics.empty()) {
    pcsampling::warn("No metric samples decoded for device %d; raw buffer size=%" PRIu64,
                     desc->device_id_, raw_size);
    return false;
  }

  // Process the metric values into stall counts
  std::map<uint64_t, EuStalls> eustalls;
  level0ProcessMetrics(metric_list, samples, metrics, eustalls);
  if (eustalls.empty()) return false;

  // Generate GPU activities based on the processed metrics and kernel properties,
  // then send these activities to the consumer
  std::deque<gpu_activity_t*> activities;
  level0GenerateActivities(kprops, eustalls, desc->correlation_id_, desc->running_kernel_,
                         activities, dispatch);
  level0SendActivities(activities);

  // Clean up the dynamically allocated activity objects
  for (auto activity : activities) {
    pcsampling::freeActivity(activity);
  }
  return true;
}

} // namespace

void*
ZeMetricProfiler::ThreadMain
(
  void* arg
)
{
  MetricProfilingThreadArgs* args = static_cast<MetricProfilingThreadArgs*>(arg);
  ZeMetricProfiler* profiler = args->profiler;
  ZeDeviceDescriptor* desc = args->descriptor;
  const struct hpcrun_foil_appdispatch_level0* dispatch = args->dispatch;
  pcsampling::freeMemory(args);
  MetricProfilingThread(profiler, desc, dispatch);
  return nullptr;
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
  if (streamer == nullptr) {
    // Initialization failed, set error state
    desc->profiler_UpdateState(PROFILER_ERROR);
    return;
  }

  // Get the list of metrics
  std::vector<std::string> metric_list;
  level0GetMetricList(group, metric_list, dispatch);
  if (!level0IsValidMetricList(metric_list)) {
    // Set error state before returning
    desc->profiler_UpdateState(PROFILER_ERROR);
    // Clean up streamer before returning
    level0CleanupMetricStreamer(context, device, group, streamer, dispatch);
    return;
  }

  RunProfilingLoop(desc, streamer, metric_list, dispatch);

  level0CleanupMetricStreamer(context, device, group, streamer, dispatch);

  // Ensure we're in a terminal state when thread exits
  // If still UNKNOWN, set to DISABLED (thread exited without setting a state)
  if (desc->profiling_state_.load(std::memory_order_acquire) == PROFILER_UNKNOWN) {
    desc->profiler_UpdateState(PROFILER_DISABLED);
  }
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
  desc->profiler_UpdateState(PROFILER_ENABLED);
  ze_result_t status;

  // Continue while profiling is enabled
  while (desc->profiler_IsActive()) {

    // Wait for the kernel to start running
    if (!WaitForKernelStart(desc)) return;

    // Update correlation ID
    int32_t device_id = desc->device_id_;
    uint64_t channel_idx = level0CorrelationChannelIndex(device_id);
    if (channel_idx >= GPU_CHANNEL_TOTAL) {
      pcsampling::warn("Correlation channel index %" PRIu64 " exceeds limit %d; falling back to base channel",
                       channel_idx, GPU_CHANNEL_TOTAL);
      channel_idx = LEVEL0_CORRELATION_CHANNEL_BASE;
    }
    pcsampling::receiveCorrelationChannel(channel_idx, level0UpdateCorrelationId, desc);

    // Wait for the next sampling interval; continuously collect metrics until the event is signaled
    if (!WaitForNextInterval(desc, dispatch, status)) return; // Rename to WaitForKernelEnd

    while (status != ZE_RESULT_SUCCESS) {
      CollectAndProcessMetrics(desc, streamer, raw_metrics, metric_list, dispatch);
      if (desc->profiler_IsDisabled()) return;
      if (!WaitForNextInterval(desc, dispatch, status)) return;
    }

    // Final sampling after the kernel has finished running
    CollectAndProcessMetrics(desc, streamer, raw_metrics, metric_list, dispatch);

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
    pcsampling::warn("No kernel properties found for device %d in level0ProcessMetricData - PC sampling data will not be processed",
                     desc->device_id_);
    return;
  }

  // Continuously process metric data while profiling is enabled
  // This ensures we drain all available data from the streamer buffer
  while (desc->profiler_IsActive()) {
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
  void* raw = pcsampling::allocMemory(sizeof(ZeMetricProfiler));
  if (!raw) {
    pcsampling::error("Failed to allocate memory for ZeMetricProfiler");
    return nullptr;
  }

  ZeMetricProfiler* profiler = static_cast<ZeMetricProfiler*>(raw);

  try {
    new (profiler) ZeMetricProfiler(dispatch);
    profiler->StartProfilingMetrics();
    return profiler;
  } catch (const std::exception& e) {
    pcsampling::error("Exception in ZeMetricProfiler::Create: %s", e.what());
    profiler->~ZeMetricProfiler();
    pcsampling::freeMemory(profiler);
    return nullptr;
  }
}

ZeMetricProfiler::ZeMetricProfiler
(
  const struct hpcrun_foil_appdispatch_level0* dispatch
)
  : dispatch_(dispatch)
{
  level0EnumerateDevices(device_descriptors_, metric_contexts_, dispatch_);
}

ZeMetricProfiler::~ZeMetricProfiler() 
{
  StopProfilingMetrics();
}

void
ZeMetricProfiler::Destroy
(
  ZeMetricProfiler* profiler
)
{
  if (!profiler) return;
  profiler->~ZeMetricProfiler();
  pcsampling::freeMemory(profiler);
}

void
ZeMetricProfiler::StartProfilingMetrics
(
  void
)
{
  for (auto it = device_descriptors_.begin(); it != device_descriptors_.end(); ++it) {
    if (it->second->parent_device_ != nullptr) {
      // Skip subdevices
      continue;
    }
    auto args = static_cast<MetricProfilingThreadArgs*>(pcsampling::allocMemory(sizeof(MetricProfilingThreadArgs)));
    if (!args) {
      pcsampling::warn("Failed to allocate thread args for profiling thread");
      it->second->app_DisableProfiler();
      continue;
    }

    args->profiler = this;
    args->descriptor = it->second;
    args->dispatch = dispatch_;

    pcsampling::disableNewThreads();
    int thread_id = pcsampling::createProfilingThread(ZeMetricProfiler::ThreadMain, args, "pcsampling");
    pcsampling::enableNewThreads();

    if (thread_id < 0) {
      pcsampling::warn("Failed to start profiling thread");
      pcsampling::freeMemory(args);
      it->second->app_DisableProfiler();
      continue;
    }

    it->second->profiling_thread_id_ = thread_id;
    // Wait for profiler initialization to complete (either success or error)
    while (!it->second->app_IsProfilerReady()) {
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    // Check if initialization failed
    if (it->second->app_IsProfilerError()) {
      pcsampling::warn("Profiler initialization failed for device %d", it->second->device_id_);
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
    it->second->app_DisableProfiler();
    
    // Join the profiling thread if it exists
    if (it->second->profiling_thread_id_ >= 0) {
      pcsampling::joinProfilingThread(it->second->profiling_thread_id_);
      it->second->profiling_thread_id_ = -1;
    }
  }

  // Clean up all device descriptors and free memory
  level0CleanupDeviceDescriptors();
  device_descriptors_.clear();

  for (auto& context : metric_contexts_) {
    if (context != nullptr) {
      ze_result_t status = pcsampling::callZeContextDestroy(context, dispatch_);
      if (status != ZE_RESULT_SUCCESS) {
        pcsampling::warn("Failed to destroy Level Zero context: %d", int(status));
      }
    }
  }
  metric_contexts_.clear();
}
