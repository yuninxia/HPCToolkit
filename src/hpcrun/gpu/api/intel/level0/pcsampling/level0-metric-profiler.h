//==============================================================
// Copyright (C) Intel Corporation
//
// SPDX-License-Identifier: MIT
// =============================================================

#ifndef PTI_TOOLS_UNITRACE_LEVEL_ZERO_METRICS_H_
#define PTI_TOOLS_UNITRACE_LEVEL_ZERO_METRICS_H_

//*****************************************************************************
// system includes
//*****************************************************************************

#include <deque>
#include <map>
#include <mutex>
#include <thread>
#include <vector>

//******************************************************************************
// level0 includes
//******************************************************************************

#include <level_zero/ze_api.h>
#include <level_zero/zet_api.h>

//*****************************************************************************
// local includes
//*****************************************************************************

#include "../../../../../libmonitor/monitor.h"
#include "../level0-command-process.h"
#include "level0-activity-generate.h"
#include "level0-activity-send.h"
#include "level0-activity-translate.h"
#include "level0-device.h"
#include "level0-kernel-properties.h"
#include "level0-metric.h"
#include "pti_assert.h"

extern "C" {
  #include "../../../../activity/correlation/gpu-correlation-channel.h"
  #include "../../../../activity/gpu-activity-channel.h"
}

//*****************************************************************************
// macros
//*****************************************************************************

constexpr static uint32_t max_metric_size = 512;
static uint32_t max_metric_samples = 65536;

#define MAX_METRIC_BUFFER  (max_metric_samples * max_metric_size * 2)

//*****************************************************************************
// type definitions
//*****************************************************************************

//*****************************************************************************
// class definition
//*****************************************************************************

class ZeMetricProfiler {
 public:
  static ZeMetricProfiler* Create(char *dir);
  ~ZeMetricProfiler();
  ZeMetricProfiler(const ZeMetricProfiler& that) = delete;
  ZeMetricProfiler& operator=(const ZeMetricProfiler& that) = delete;

  void GetDeviceDescriptors(std::map<ze_device_handle_t, ZeDeviceDescriptor*>& out_descriptors);

  void InsertCommandListDeviceMapping(ze_command_list_handle_t cmdList, ze_device_handle_t device) {
    std::lock_guard<std::mutex> lock(cmdlist_device_map_mutex_);
    cmdlist_device_map_[cmdList] = device;
  }

  ze_device_handle_t GetDeviceForCommandList(ze_command_list_handle_t cmdList) {
    std::lock_guard<std::mutex> lock(cmdlist_device_map_mutex_);
    auto it = cmdlist_device_map_.find(cmdList);
    if (it != cmdlist_device_map_.end()) {
      return it->second;
    }
    return nullptr;
  }

 private:
  ZeMetricProfiler();
  void StartProfilingMetrics();
  void StopProfilingMetrics();

  static void MetricProfilingThread(ZeMetricProfiler* profiler, ZeDeviceDescriptor *desc);
  static void RunProfilingLoop(ZeMetricProfiler* profiler, ZeDeviceDescriptor* desc, zet_metric_streamer_handle_t& streamer);
  static void CollectAndProcessMetrics(ZeMetricProfiler* profiler, ZeDeviceDescriptor* desc, zet_metric_streamer_handle_t& streamer);
  static void FlushStreamerBuffer(zet_metric_streamer_handle_t& streamer, ZeDeviceDescriptor* desc);
  static void UpdateCorrelationID(uint64_t cid, gpu_activity_channel_t *channel, void *arg);

 private: // Data
  static std::string data_dir_name_;
  std::vector<ze_context_handle_t> metric_contexts_;
  std::map<ze_device_handle_t, ZeDeviceDescriptor *> device_descriptors_;
  std::mutex cmdlist_device_map_mutex_;
  std::map<ze_command_list_handle_t, ze_device_handle_t> cmdlist_device_map_;
};

std::string ZeMetricProfiler::data_dir_name_;

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

ZeMetricProfiler::ZeMetricProfiler
(
  void
) 
{
  zeroEnumerateDevices(device_descriptors_, metric_contexts_);
}

ZeMetricProfiler::~ZeMetricProfiler
(
  void
) 
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
    PTI_ASSERT(it->second->profiling_thread_ != nullptr);
    PTI_ASSERT(it->second->profiling_state_ == PROFILER_ENABLED);
    it->second->profiling_state_.store(PROFILER_DISABLED, std::memory_order_release);
    it->second->profiling_thread_->join();
    delete it->second->profiling_thread_;
    it->second->profiling_thread_ = nullptr;
  }
  device_descriptors_.clear();
}

void
ZeMetricProfiler::GetDeviceDescriptors
(
  std::map<ze_device_handle_t, ZeDeviceDescriptor*>& out_descriptors
)
{
  out_descriptors = device_descriptors_;
}

void 
ZeMetricProfiler::UpdateCorrelationID
(
  uint64_t cid, 
  gpu_activity_channel_t *channel, 
  void *arg
) 
{
  ZeDeviceDescriptor* desc = static_cast<ZeDeviceDescriptor*>(arg);
  desc->correlation_id_ = cid;
}

void
ZeMetricProfiler::FlushStreamerBuffer
(
  zet_metric_streamer_handle_t& streamer,
  ZeDeviceDescriptor* desc
)
{
  ze_result_t status = ZE_RESULT_SUCCESS;

  // Close the old streamer
  status = zetMetricStreamerClose(streamer);
  PTI_ASSERT(status == ZE_RESULT_SUCCESS);

  // Open a new streamer
  uint32_t interval = 100 * 1000; // ns
  zet_metric_streamer_desc_t streamer_desc = {ZET_STRUCTURE_TYPE_METRIC_STREAMER_DESC, nullptr, max_metric_samples, interval};
  status = zetMetricStreamerOpen(desc->context_, desc->device_, desc->metric_group_, &streamer_desc, nullptr, &streamer);
  if (status != ZE_RESULT_SUCCESS) {
    std::cerr << "[ERROR] Failed to open metric streamer (" << status << "). The sampling interval might be too small." << std::endl;
    streamer = nullptr; // Make sure to set streamer to nullptr if fails
    return;
  }

  if (streamer_desc.notifyEveryNReports > max_metric_samples) {
    max_metric_samples = streamer_desc.notifyEveryNReports;
  }
}

void 
ZeMetricProfiler::CollectAndProcessMetrics
(
  ZeMetricProfiler* profiler, 
  ZeDeviceDescriptor* desc,
  zet_metric_streamer_handle_t& streamer
)
{
  std::vector<uint8_t> raw_metrics(MAX_METRIC_BUFFER + 512);
  uint64_t raw_size;
  zeroCollectMetrics(streamer, raw_metrics, raw_size);
  if (raw_size == 0) return;

  std::map<uint64_t, EuStalls> eustalls;
  zeroCalculateEuStalls(desc->metric_group_, raw_size, raw_metrics, eustalls);
  if (eustalls.size() == 0) return;

  std::map<uint64_t, KernelProperties> kprops;
  zeroReadKernelProperties(desc->device_id_, data_dir_name_, kprops);
  if (kprops.size() == 0) return;

  std::deque<gpu_activity_t*> activities;
  zeroGenerateActivities(kprops, eustalls, desc->correlation_id_, activities);
  zeroSendActivities(activities);

#if 0
  zeroLogActivities(activities, kprops);
#endif

  for (auto activity : activities) {
    delete activity;
  }
  activities.clear();
}

void 
ZeMetricProfiler::RunProfilingLoop
(
  ZeMetricProfiler* profiler,
  ZeDeviceDescriptor* desc,
  zet_metric_streamer_handle_t& streamer
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

      if (desc->profiling_state_.load(std::memory_order_acquire) == PROFILER_DISABLED) {
        return;
      }
    }

    // Kernel is running, enter sampling loop
    while (true) {
      // Update correlation ID
      gpu_correlation_channel_receive(1, UpdateCorrelationID, desc);

      CollectAndProcessMetrics(profiler, desc, streamer);
      FlushStreamerBuffer(streamer, desc);

      // Wait for the next interval
      status = zeEventHostSynchronize(desc->serial_kernel_start_, 50000000);
      if (status != ZE_RESULT_SUCCESS) {
        break;
      }
    }

    // Kernel has finished, perform final sampling and cleanup
    CollectAndProcessMetrics(profiler, desc, streamer);
    FlushStreamerBuffer(streamer, desc);

    status = zeEventHostReset(desc->serial_kernel_start_);
    PTI_ASSERT(status == ZE_RESULT_SUCCESS);
    
    // Notify the app thread that data processing is complete
    status = zeEventHostSignal(desc->serial_data_ready_);
    PTI_ASSERT(status == ZE_RESULT_SUCCESS);
  }
}

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
  PTI_ASSERT(status == ZE_RESULT_SUCCESS);

  zet_metric_streamer_handle_t streamer = nullptr;
  uint32_t interval = 100 * 1000; // ns
  uint32_t notifyEveryNReports = 1024;

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
  PTI_ASSERT(!metric_list.empty());

  RunProfilingLoop(profiler, desc, streamer);

  status = zetMetricStreamerClose(streamer);
  PTI_ASSERT(status == ZE_RESULT_SUCCESS);

  status = zetContextActivateMetricGroups(context, device, 0, &group);
  PTI_ASSERT(status == ZE_RESULT_SUCCESS);
}

#endif // PTI_TOOLS_UNITRACE_LEVEL_ZERO_METRICS_H_