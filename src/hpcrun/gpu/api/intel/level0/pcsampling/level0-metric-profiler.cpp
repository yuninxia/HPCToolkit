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
// global variables
//******************************************************************************

uint32_t max_metric_samples = 65536;


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
  ZeDeviceDescriptor *desc
)
{
  ze_result_t status = ZE_RESULT_SUCCESS;

  ze_context_handle_t context = desc->context_;
  ze_device_handle_t device = desc->device_;
  zet_metric_group_handle_t group = desc->metric_group_;

  status = zetContextActivateMetricGroups(context, device, 1, &group);
  level0_check_result(status, __LINE__);

  zet_metric_streamer_handle_t streamer = nullptr;
  uint32_t interval = 500000; // ns
  uint32_t notifyEveryNReports = 65536;

  zet_metric_streamer_desc_t streamer_desc = { ZET_STRUCTURE_TYPE_METRIC_STREAMER_DESC, nullptr, notifyEveryNReports, interval };
  status = zetMetricStreamerOpen(context, device, group, &streamer_desc, nullptr, &streamer);
  if (status != ZE_RESULT_SUCCESS) {
    std::cerr << "[ERROR] Failed to open metric streamer (" << status << "). The sampling interval might be too small." << std::endl;

    // set state to enabled to let the parent thread continue
    desc->profiling_state_.store(PROFILER_ENABLED, std::memory_order_release);
    return;
  }

  if (streamer_desc.notifyEveryNReports > max_metric_samples) {
    max_metric_samples = streamer_desc.notifyEveryNReports;
  }

  std::vector<std::string> metric_list;
  zeroGetMetricList(group, metric_list);
  if (!zeroIsValidMetricList(metric_list)) return;

  RunProfilingLoop(desc, streamer, metric_list);

  status = zetMetricStreamerClose(streamer);
  level0_check_result(status, __LINE__);

  status = zetContextActivateMetricGroups(context, device, 0, &group);
  level0_check_result(status, __LINE__);
}

void 
ZeMetricProfiler::RunProfilingLoop
(
  ZeDeviceDescriptor* desc,
  zet_metric_streamer_handle_t& streamer,
  std::vector<std::string>& metric_list
)
{
  std::vector<uint8_t> raw_metrics(MAX_METRIC_BUFFER + 512);
  desc->profiling_state_.store(PROFILER_ENABLED, std::memory_order_release);
  ze_result_t status;
  
  while (desc->profiling_state_.load(std::memory_order_acquire) != PROFILER_DISABLED) {
    // Wait for the kernel to start running
    while (true) {
      status = zeEventHostSynchronize(desc->serial_kernel_start_, 50000000);
      if (status == ZE_RESULT_SUCCESS) {
        break;
      }

      // Handle case where kernel execution is extremely short:
      // In such cases, the kernel might finish before zeEventHostSynchronize can detect the start event.
      // Without this check, a deadlock could occur:
      // - The Profiling thread would keep waiting for the start event (which has already been reset).
      // - The App thread would be waiting for the Profiling thread to complete data processing.
      // kernel_started_ allows Profiling thread to proceed, avoiding deadlock.
      if (desc->kernel_started_.load(std::memory_order_acquire)) {
        break;
      }

      if (desc->profiling_state_.load(std::memory_order_acquire) == PROFILER_DISABLED) {
        return;
      }
    }

    // Kernel is running, enter sampling loop
    while (true) {
      // Update correlation ID
      gpu_correlation_channel_receive(1, zeroUpdateCorrelationId, desc);

      // Wait for the next interval
      status = zeEventHostSynchronize(desc->serial_kernel_end_, 5000);
      if (status == ZE_RESULT_SUCCESS) {
        break;
      }

      CollectAndProcessMetrics(desc, streamer, raw_metrics, metric_list);
    }

    // Kernel has finished, perform final sampling and cleanup
    CollectAndProcessMetrics(desc, streamer, raw_metrics, metric_list);

#if 0
    // FIXME(Yuning): need a better way to flush the streamer buffer without repeatedly closing and reopening the streamer
    zeroFlushStreamerBuffer(streamer, desc);
#endif

    desc->running_kernel_ = nullptr;
    desc->kernel_started_.store(false, std::memory_order_release);
    
    // Notify the app thread that data processing is complete
    status = zeEventHostSignal(desc->serial_data_ready_);
    level0_check_result(status, __LINE__);
  }
}

void
ZeMetricProfiler::CollectAndProcessMetrics
(
  ZeDeviceDescriptor* desc,
  zet_metric_streamer_handle_t& streamer,
  std::vector<uint8_t>& raw_metrics,
  std::vector<std::string>& metric_list
)
{ 
  uint64_t raw_size;
  zeroMetricStreamerReadData(streamer, raw_metrics, raw_size);
  if (raw_size == 0) return;

  // Calculate multiple sets of metric values, potentially one set per sub-device
  // samples: Number of metric value sets (typically one per sub-device)
  // metrics: Concatenated metric values for all sets/sub-devices
  std::vector<uint32_t> samples;
  std::vector<zet_typed_value_t> metrics;
  zeroMetricGroupCalculateMultipleMetricValuesExp(desc->metric_group_, raw_size, raw_metrics, samples, metrics);
  if (samples.empty() || metrics.empty()) return;

  std::map<uint64_t, EuStalls> eustalls;
  zeroProcessMetrics(metric_list, samples, metrics, eustalls);
  if (eustalls.empty()) return;

  std::map<uint64_t, KernelProperties> kprops;
  zeroReadKernelProperties(desc->device_id_, data_dir_name_, kprops);
  if (kprops.empty()) return;

  std::deque<gpu_activity_t*> activities;
  zeroGenerateActivities(kprops, eustalls, desc->correlation_id_, activities, desc->running_kernel_);
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


//******************************************************************************
// public methods
//******************************************************************************

ZeMetricProfiler* 
ZeMetricProfiler::Create
(
  char *dir
)
{
  data_dir_name_ = std::string(dir);
  ZeMetricProfiler* profiler = new ZeMetricProfiler();
  profiler->StartProfilingMetrics();
  return profiler;
}

ZeMetricProfiler::ZeMetricProfiler() 
{
  zeroEnumerateDevices(device_descriptors_, metric_contexts_);
}

ZeMetricProfiler::~ZeMetricProfiler() 
{
  StopProfilingMetrics();
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
    monitor_disable_new_threads();
    it->second->profiling_thread_ = new std::thread(MetricProfilingThread, this, it->second);
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
