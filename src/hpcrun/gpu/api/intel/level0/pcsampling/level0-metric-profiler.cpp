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
// local variables
//******************************************************************************

static std::atomic<int> profiler_thread_counter{500};


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
  ze_context_handle_t context = desc->context_;
  ze_device_handle_t device = desc->device_;
  zet_metric_group_handle_t group = desc->metric_group_;
  zet_metric_streamer_handle_t streamer = nullptr;

  zeroInitializeMetricStreamer(context, device, group, streamer);

  std::vector<std::string> metric_list;
  zeroGetMetricList(group, metric_list);
  if (!zeroIsValidMetricList(metric_list)) return;

  if (concurrent_metric_profiling) {
    profiler->RunConcurrentProfilingLoop(desc, streamer, metric_list);
  } else {
    profiler->RunSequentialProfilingLoop(desc, streamer, metric_list);
  }

  zeroCleanupMetricStreamer(context, device, group, streamer);
}

void
ZeMetricProfiler::RunConcurrentProfilingLoop
(
  ZeDeviceDescriptor* desc,
  zet_metric_streamer_handle_t& streamer,
  std::vector<std::string>& metric_list
)
{
  std::vector<uint8_t> raw_metrics(MAX_METRIC_BUFFER + 512);
  desc->profiling_state_.store(PROFILER_ENABLED, std::memory_order_release);
  uint64_t ssize = MAX_METRIC_BUFFER + 512;
  
  // Create a persistent map to accumulate EuStalls across all samples
  std::map<uint64_t, EuStalls> accumulated_eustalls;
  
  while (desc->profiling_state_.load(std::memory_order_acquire) != PROFILER_DISABLED) {
    zeroAccumulateEUStallMetrics(desc->metric_group_, streamer, raw_metrics, ssize, metric_list, accumulated_eustalls);
  }

  // Final collection after loop ends
  zeroAccumulateEUStallMetrics(desc->metric_group_, streamer, raw_metrics, ssize, metric_list, accumulated_eustalls);

  if (accumulated_eustalls.empty()) return;
   
  // Read kernel properties
  std::map<uint64_t, KernelProperties> kprops;
  zeroReadKernelProperties(desc->device_id_, data_dir_name_, kprops);
  if (kprops.empty()) return;
  
  // Generate activities using the complete accumulated data
  std::deque<gpu_activity_t*> activities;
  zeroGenerateActivitiesFromAccumulatedMetrics(kprops, accumulated_eustalls, kernel_timing_data_, activities);

#if 0
  zeroLogActivities(activities, kprops);
#endif

  int thread_id = profiler_thread_counter.fetch_add(1);
  bool demand_new_thread = true;

  thread_data_t* td = zeroInitThreadData(thread_id, demand_new_thread);
  if (!td) return;

  zeroInitIdTuple(td, desc->device_id_, thread_id);

  if (!zeroBuildCCT(td, activities)) {
    std::cerr << "Failed to build CCT" << std::endl;
    return;
  }
}

void 
ZeMetricProfiler::RunSequentialProfilingLoop
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
      if (status == ZE_RESULT_SUCCESS) break;

      // Handle case where kernel execution is extremely short:
      // In such cases, the kernel might finish before zeEventHostSynchronize can detect the start event.
      // Without this check, a deadlock could occur:
      // - The Profiling thread would keep waiting for the start event (which has already been reset).
      // - The App thread would be waiting for the Profiling thread to complete data processing.
      // kernel_started_ allows Profiling thread to proceed, avoiding deadlock.
      if (desc->kernel_started_.load(std::memory_order_acquire)) break;

      if (desc->profiling_state_.load(std::memory_order_acquire) == PROFILER_DISABLED) return;
    }

    // Kernel is running, enter sampling loop
    while (true) {
      // Update correlation ID
      gpu_correlation_channel_receive(1, zeroUpdateCorrelationId, desc);

      // Wait for the next interval
      status = zeEventHostSynchronize(desc->serial_kernel_end_, 5000);
      if (status == ZE_RESULT_SUCCESS) break;

      CollectAndProcessMetrics(desc, streamer, raw_metrics, metric_list);
    }

    // Kernel has finished, perform final sampling and cleanup
    CollectAndProcessMetrics(desc, streamer, raw_metrics, metric_list);

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
  std::map<uint64_t, KernelProperties> kprops;
  zeroReadKernelProperties(desc->device_id_, data_dir_name_, kprops);
  if (kprops.empty()) return;

  uint64_t ssize = MAX_METRIC_BUFFER + 512;
  
  while (desc->profiling_state_.load(std::memory_order_acquire) != PROFILER_DISABLED) {
    uint64_t raw_size = zeroMetricStreamerReadData(streamer, raw_metrics, ssize);
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
