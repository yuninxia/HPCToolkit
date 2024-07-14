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
#include "level0-notify.h"
#include "level0-sync.h"
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
  static void RunProfilingLoop(ZeMetricProfiler* profiler, ZeDeviceDescriptor* desc, ze_event_handle_t event, ze_event_pool_handle_t event_pool, zet_metric_streamer_handle_t& streamer);
  static void CollectAndProcessMetrics(ZeMetricProfiler* profiler, ZeDeviceDescriptor* desc, zet_metric_streamer_handle_t& streamer);
  static void FlushStreamerBuffer(zet_metric_streamer_handle_t& streamer, ZeDeviceDescriptor* desc, ze_event_handle_t event, ze_event_pool_handle_t event_pool);
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

    pthread_mutex_destroy(&it->second->kernel_mutex_);
    pthread_cond_destroy(&it->second->kernel_cond_);
    
    pthread_mutex_destroy(&it->second->data_mutex_);
    pthread_cond_destroy(&it->second->data_cond_);
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
  ZeDeviceDescriptor* desc,
  ze_event_handle_t event,
  ze_event_pool_handle_t event_pool
)
{
  ze_result_t status = ZE_RESULT_SUCCESS;

  // Close the old streamer
  status = zetMetricStreamerClose(streamer);
  PTI_ASSERT(status == ZE_RESULT_SUCCESS);

  // Open a new streamer
  uint32_t interval = 50 * 1000; // ns
  zet_metric_streamer_desc_t streamer_desc = {ZET_STRUCTURE_TYPE_METRIC_STREAMER_DESC, nullptr, max_metric_samples, interval};
  status = zetMetricStreamerOpen(desc->context_, desc->device_, desc->metric_group_, &streamer_desc, event, &streamer);
  if (status != ZE_RESULT_SUCCESS) {
    std::cerr << "[ERROR] Failed to open metric streamer (" << status << "). The sampling interval might be too small." << std::endl;
    status = zeEventDestroy(event);
    PTI_ASSERT(status == ZE_RESULT_SUCCESS);
    status = zeEventPoolDestroy(event_pool);
    PTI_ASSERT(status == ZE_RESULT_SUCCESS);
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

#if 1
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
  ze_event_handle_t event, 
  ze_event_pool_handle_t event_pool,
  zet_metric_streamer_handle_t& streamer
) 
{
  std::vector<uint8_t> raw_metrics(MAX_METRIC_BUFFER + 512);
  desc->profiling_state_.store(PROFILER_ENABLED, std::memory_order_release);
  
  while (desc->profiling_state_.load(std::memory_order_acquire) != PROFILER_DISABLED) {
    pthread_mutex_lock(&desc->kernel_mutex_);
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec += 1;
    
    // Wait for the kernel to start running
    while (!desc->kernel_running_) {
      int rc = pthread_cond_timedwait(&desc->kernel_cond_, &desc->kernel_mutex_, &ts);
      if (rc == ETIMEDOUT) {
        // Check if profiling should be terminated
        if (desc->profiling_state_.load(std::memory_order_acquire) == PROFILER_DISABLED) {
          pthread_mutex_unlock(&desc->kernel_mutex_);
          return;
        }
        // Reset timeout
        clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_sec += 1;
      } else if (rc != 0) {
        // Handle unexpected errors
        fprintf(stderr, "pthread_cond_timedwait error: %d\n", rc);
        pthread_mutex_unlock(&desc->kernel_mutex_);
        return;
      }
    }
    pthread_mutex_unlock(&desc->kernel_mutex_);

    // Kernel is running, enter sampling loop
    while (true) {
      // Update correlation ID
      gpu_correlation_channel_receive(1, UpdateCorrelationID, desc);
      
      // Wait for the event with a timeout
      ze_result_t status = zeEventHostSynchronize(event, 50000000 /* wait delay in nanoseconds */);
      PTI_ASSERT(status == ZE_RESULT_SUCCESS || status == ZE_RESULT_NOT_READY);
      
      if (status == ZE_RESULT_SUCCESS) {
        // Event is signaled, process it
        status = zeEventHostReset(event);
        PTI_ASSERT(status == ZE_RESULT_SUCCESS);
        CollectAndProcessMetrics(profiler, desc, streamer);
        FlushStreamerBuffer(streamer, desc, event, event_pool);
      }

      // Check if the kernel has finished
      pthread_mutex_lock(&desc->kernel_mutex_);
      bool is_kernel_finished = !desc->kernel_running_;
      pthread_mutex_unlock(&desc->kernel_mutex_);

      if (is_kernel_finished) {
        break;  // Exit sampling loop
      }
    }

    // Kernel has finished, perform final sampling and cleanup
    ze_result_t status = zeEventHostReset(event);
    PTI_ASSERT(status == ZE_RESULT_SUCCESS);
    CollectAndProcessMetrics(profiler, desc, streamer);
    FlushStreamerBuffer(streamer, desc, event, event_pool);
    
    // Notify the app thread that data processing is complete
    zeroNotifyDataProcessingComplete(desc);
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

  ze_event_pool_handle_t event_pool = nullptr;
  ze_event_pool_desc_t event_pool_desc = { ZE_STRUCTURE_TYPE_EVENT_POOL_DESC, nullptr, ZE_EVENT_POOL_FLAG_HOST_VISIBLE, 1 };
  status = zeEventPoolCreate(context, &event_pool_desc, 1, &device, &event_pool);
  PTI_ASSERT(status == ZE_RESULT_SUCCESS);

  ze_event_handle_t event = nullptr;
  ze_event_desc_t event_desc = { ZE_STRUCTURE_TYPE_EVENT_DESC, nullptr, 0, ZE_EVENT_SCOPE_FLAG_HOST, ZE_EVENT_SCOPE_FLAG_HOST };
  status = zeEventCreate(event_pool, &event_desc, &event);
  PTI_ASSERT(status == ZE_RESULT_SUCCESS);

  zet_metric_streamer_handle_t streamer = nullptr;
  uint32_t interval = 50 * 1000; // ns
  uint32_t notifyEveryNReports = 1024;

  zet_metric_streamer_desc_t streamer_desc = { ZET_STRUCTURE_TYPE_METRIC_STREAMER_DESC, nullptr, notifyEveryNReports, interval };
  status = zetMetricStreamerOpen(context, device, group, &streamer_desc, event, &streamer);
  if (status != ZE_RESULT_SUCCESS) {
    std::cerr << "[ERROR] Failed to open metric streamer (" << status << "). The sampling interval might be too small." << std::endl;

    status = zeEventDestroy(event);
    PTI_ASSERT(status == ZE_RESULT_SUCCESS);

    status = zeEventPoolDestroy(event_pool);
    PTI_ASSERT(status == ZE_RESULT_SUCCESS);

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

  RunProfilingLoop(profiler, desc, event, event_pool, streamer);

  status = zetMetricStreamerClose(streamer);
  PTI_ASSERT(status == ZE_RESULT_SUCCESS);

  status = zeEventDestroy(event);
  PTI_ASSERT(status == ZE_RESULT_SUCCESS);

  status = zeEventPoolDestroy(event_pool);
  PTI_ASSERT(status == ZE_RESULT_SUCCESS);

  status = zetContextActivateMetricGroups(context, device, 0, &group);
  PTI_ASSERT(status == ZE_RESULT_SUCCESS);
}

#endif // PTI_TOOLS_UNITRACE_LEVEL_ZERO_METRICS_H_