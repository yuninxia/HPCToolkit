#ifndef LEVEL0_DEVICE_H
#define LEVEL0_DEVICE_H

#include <level_zero/ze_api.h>
#include <level_zero/zet_api.h>

#include <atomic>
#include <cstring>
#include <iostream>
#include <map>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include "level0-metric.h"
#include "pti_assert.h"

enum ZeProfilerState {
  PROFILER_DISABLED = 0,
  PROFILER_ENABLED = 1
};

struct ZeDeviceDescriptor {
  ze_device_handle_t device_;
  ze_device_handle_t parent_device_;
  ze_driver_handle_t driver_;
  ze_context_handle_t context_;
  int32_t device_id_;
  int32_t parent_device_id_;
  int32_t subdevice_id_;
  int32_t num_sub_devices_;
  zet_metric_group_handle_t metric_group_;
  std::thread *profiling_thread_;
  std::atomic<ZeProfilerState> profiling_state_;
  bool stall_sampling_;
  pthread_mutex_t kernel_mutex_;
  pthread_cond_t kernel_cond_;
  bool kernel_running_;
  pthread_mutex_t data_mutex_;
  pthread_cond_t data_cond_;
  bool data_processed_;
  uint64_t correlation_id_;
  uint64_t last_correlation_id_;
};

ZeDeviceDescriptor*
zeroCreateDeviceDescriptor
(
  ze_device_handle_t device, 
  int32_t did, 
  ze_driver_handle_t driver, 
  ze_context_handle_t context, 
  bool stall_sampling, 
  const std::string& metric_group
);

uint32_t
zeroGetSubDeviceCount
(
  ze_device_handle_t device
);

void 
zeroHandleSubDevices
(
  ZeDeviceDescriptor* parent_desc, 
  std::map<ze_device_handle_t, 
  ZeDeviceDescriptor*>& device_descriptors
);

void 
zeroEnumerateDevices
(
  std::map<ze_device_handle_t, ZeDeviceDescriptor*>& device_descriptors_,
  std::vector<ze_context_handle_t>& metric_contexts
);

int 
zeroGetDeviceId
(
  const std::map<ze_device_handle_t, ZeDeviceDescriptor*>& device_descriptors, 
  ze_device_handle_t sub_device
);

int
zeroGetSubDeviceId
(
  const std::map<ze_device_handle_t, ZeDeviceDescriptor*>& device_descriptors, 
  ze_device_handle_t sub_device
);

void
zeroGetParentDevice
(
  const std::map<ze_device_handle_t, ZeDeviceDescriptor*>& device_descriptors,
  ze_device_handle_t sub_device,
  ze_device_handle_t& parent_device
);

ze_device_handle_t
zeroConvertToRootDevice
(
  ze_device_handle_t device
);

#endif  // LEVEL0_DEVICE_H