// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: BSD-3-Clause

// -*-Mode: C++;-*-

//*****************************************************************************
// local includes
//*****************************************************************************

#include "level0-metric-profiler.hpp"


//******************************************************************************
// static member variables
//******************************************************************************

std::string ZeMetricProfiler::data_dir_name_;


//******************************************************************************
// private methods
//******************************************************************************

static bool
WaitForKernelStart
(
  ZeDeviceDescriptor* desc
)
{
  while (true) {
    if (desc->kernel_started_.load(std::memory_order_acquire)) return true;
    if (desc->profiling_state_.load(std::memory_order_acquire) == PROFILER_DISABLED) return false;
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
  while (true) {
    status = f_zeEventQueryStatus(desc->running_kernel_end_, dispatch);
    if (status == ZE_RESULT_SUCCESS) return true;
    if (desc->profiling_state_.load(std::memory_order_acquire) == PROFILER_DISABLED) return false;
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
  if (samples.empty() || metrics.empty()) return false;

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
    delete activity;
  }
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
  if (!level0IsValidMetricList(metric_list)) return;

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
  desc->profiling_state_.store(PROFILER_ENABLED, std::memory_order_release);
  ze_result_t status;

  // Continue while profiling is enabled
  while (desc->profiling_state_.load(std::memory_order_acquire) != PROFILER_DISABLED) {

    // Wait for the kernel to start running
    if (!WaitForKernelStart(desc)) return;

    // Update correlation ID
    gpu_correlation_channel_receive(1, level0UpdateCorrelationId, desc);

    // Wait for the next sampling interval; continuously collect metrics until the event is signaled
    if (!WaitForNextInterval(desc, dispatch, status)) return;

    while (status != ZE_RESULT_SUCCESS) {
      CollectAndProcessMetrics(desc, streamer, raw_metrics, metric_list, dispatch);
      if (desc->profiling_state_.load(std::memory_order_acquire) == PROFILER_DISABLED) return;
      if (!WaitForNextInterval(desc, dispatch, status)) return;
    }

    // Final sampling after the kernel has finished running
    CollectAndProcessMetrics(desc, streamer, raw_metrics, metric_list, dispatch);

    // Reset kernel state
    desc->running_kernel_ = nullptr;
    desc->kernel_started_.store(false, std::memory_order_release);

    // Notify the application thread that data processing is complete
    desc->serial_data_ready_.store(true, std::memory_order_release);
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
  // Read kernel properties from file
  std::map<uint64_t, KernelProperties> kprops;
  level0ReadKernelProperties(desc->device_id_, data_dir_name_, kprops);
  if (kprops.empty()) return;

  // Continuously process metric data while profiling is enabled
  while (desc->profiling_state_.load(std::memory_order_acquire) != PROFILER_DISABLED) {
    if (!ProcessMetricData(desc, streamer, raw_metrics, metric_list, kprops, dispatch))
      return;
  }
}


//******************************************************************************
// public methods
//******************************************************************************

ZeMetricProfiler*
ZeMetricProfiler::Create
(
  char* dir,
  const struct hpcrun_foil_appdispatch_level0* dispatch
)
{
  data_dir_name_ = std::string(dir);
  ZeMetricProfiler* profiler = new ZeMetricProfiler(dispatch);
  profiler->StartProfilingMetrics(dispatch);
  return profiler;
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
    monitor_disable_new_threads();
    it->second->profiling_thread_ = new std::thread(MetricProfilingThread, this, it->second, dispatch);
    monitor_enable_new_threads();
    // Wait until profiling is enabled before continuing
    while (it->second->profiling_state_.load(std::memory_order_acquire) != PROFILER_ENABLED) {
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
    assert(it->second->profiling_thread_ != nullptr);
    assert(it->second->profiling_state_ == PROFILER_ENABLED);
    it->second->profiling_state_.store(PROFILER_DISABLED, std::memory_order_release);
    it->second->profiling_thread_->join();
    delete it->second->profiling_thread_;
    it->second->profiling_thread_ = nullptr;
  }
  device_descriptors_.clear();
}
