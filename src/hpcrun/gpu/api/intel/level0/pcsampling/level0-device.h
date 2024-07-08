#ifndef LEVEL0_DEVICE_H
#define LEVEL0_DEVICE_H

#include <level_zero/ze_api.h>
#include <level_zero/zet_api.h>
#include <atomic>
#include <map>
#include <string>
#include <thread>
#include <vector>

namespace l0_device {

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
};

ZeDeviceDescriptor*
CreateDeviceDescriptor
(
  ze_device_handle_t device, 
  int32_t did, 
  ze_driver_handle_t driver, 
  ze_context_handle_t context, 
  bool stall_sampling, 
  const std::string& metric_group
);

uint32_t 
GetSubDeviceCount
(
  ze_device_handle_t device
);

void 
HandleSubDevices
(
  ZeDeviceDescriptor* parent_desc, 
  std::map<ze_device_handle_t, 
  ZeDeviceDescriptor*>& device_descriptors
);

void 
EnumerateDevices
(
  std::map<ze_device_handle_t, ZeDeviceDescriptor*>& device_descriptors_, 
  std::vector<ze_context_handle_t>& metric_contexts
);

int 
GetDeviceId
(
  const std::map<ze_device_handle_t, ZeDeviceDescriptor*>& device_descriptors, 
  ze_device_handle_t sub_device
);

int 
GetSubDeviceId
(
  const std::map<ze_device_handle_t, ZeDeviceDescriptor*>& device_descriptors, 
  ze_device_handle_t sub_device
);

ze_device_handle_t 
GetParentDevice
(
  const std::map<ze_device_handle_t, ZeDeviceDescriptor*>& device_descriptors, 
  ze_device_handle_t sub_device
);

}  // namespace l0_device

#endif  // LEVEL0_DEVICE_H