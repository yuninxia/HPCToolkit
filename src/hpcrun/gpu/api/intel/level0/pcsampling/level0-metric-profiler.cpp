// SPDX-FileCopyrightText: 2002-2024 Rice University
// SPDX-FileCopyrightText: 2024 Contributors to the HPCToolkit Project
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

void 
ZeMetricProfiler::MetricProfilingThread
(
  ZeMetricProfiler* profiler, 
  ZeDeviceDescriptor *desc,
  const struct hpcrun_foil_appdispatch_level0* dispatch
)
{
  ze_context_handle_t context = desc->context_;
  ze_device_handle_t device = desc->device_;
  zet_metric_group_handle_t group = desc->metric_group_;
  zet_metric_streamer_handle_t streamer = nullptr;

  zeroInitializeMetricStreamer(context, device, group, streamer, dispatch);

  std::vector<std::string> metric_list;
  zeroGetMetricList(group, metric_list, dispatch);
  if (!zeroIsValidMetricList(metric_list)) return;

  RunProfilingLoop(desc, streamer, metric_list, dispatch);

  zeroCleanupMetricStreamer(context, device, group, streamer, dispatch);
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
  
  while (desc->profiling_state_.load(std::memory_order_acquire) != PROFILER_DISABLED) {
    // Wait for the kernel to start running
    while (true) {

      if (desc->kernel_started_.load(std::memory_order_acquire)) break;

      if (desc->profiling_state_.load(std::memory_order_acquire) == PROFILER_DISABLED) return;
      
      std::this_thread::yield();
    }

    // Update correlation ID
    gpu_correlation_channel_receive(1, zeroUpdateCorrelationId, desc);

    // Kernel is running, enter sampling loop
    while (true) {
      // Wait for the next interval
      status = f_zeEventQueryStatus(desc->running_kernel_end_, dispatch);
      if (status == ZE_RESULT_SUCCESS) break;

      CollectAndProcessMetrics(desc, streamer, raw_metrics, metric_list, dispatch);

      if (desc->profiling_state_.load(std::memory_order_acquire) == PROFILER_DISABLED) return;

      std::this_thread::yield();
    }

    // Kernel has finished, perform final sampling and cleanup
    CollectAndProcessMetrics(desc, streamer, raw_metrics, metric_list, dispatch);

    desc->running_kernel_ = nullptr;
    desc->kernel_started_.store(false, std::memory_order_release);
    
    // Notify the app thread that data processing is complete
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
  std::map<uint64_t, KernelProperties> kprops;
  zeroReadKernelProperties(desc->device_id_, data_dir_name_, kprops);
  if (kprops.empty()) return;

  uint64_t ssize = MAX_METRIC_BUFFER + 512;
  
  while (desc->profiling_state_.load(std::memory_order_acquire) != PROFILER_DISABLED) {
    uint64_t raw_size = zeroMetricStreamerReadData(streamer, raw_metrics, ssize, dispatch);
    if (raw_size == 0) return;

    // Calculate multiple sets of metric values, potentially one set per sub-device
    // samples: Number of metric value sets (typically one per sub-device)
    // metrics: Concatenated metric values for all sets/sub-devices
    std::vector<uint32_t> samples;
    std::vector<zet_typed_value_t> metrics;
    zeroMetricGroupCalculateMultipleMetricValuesExp(desc->metric_group_, raw_size, raw_metrics, samples, metrics, dispatch);
    if (samples.empty() || metrics.empty()) return;

    std::map<uint64_t, EuStalls> eustalls;
    zeroProcessMetrics(metric_list, samples, metrics, eustalls);
    if (eustalls.empty()) return;

    std::deque<gpu_activity_t*> activities;
    zeroGenerateActivities(kprops, eustalls, desc->correlation_id_, desc->running_kernel_, activities, dispatch);
    zeroSendActivities(activities);

#if 0
    zeroLogSamplesAndMetrics(samples, metrics);
    zeroLogMetricList(metric_list);
    zeroLogActivities(activities, kprops);
#endif

    for (auto activity : activities) {
      delete activity;
    }
    activities.clear();
  }
}


//******************************************************************************
// public methods
//******************************************************************************

ZeMetricProfiler* 
ZeMetricProfiler::Create
(
  char *dir,
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
  zeroEnumerateDevices(device_descriptors_, metric_contexts_, dispatch);
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
