#include "level0-device.h"
#include "level0-metric.h"
#include "pti_assert.h"
#include <iostream>

namespace l0_device {

ZeDeviceDescriptor*
CreateDeviceDescriptor
(
  ze_device_handle_t device, 
  int32_t did, 
  ze_driver_handle_t driver, 
  ze_context_handle_t context, 
  bool stall_sampling, 
  const std::string& metric_group
) 
{
  ZeDeviceDescriptor *desc = new ZeDeviceDescriptor;

  desc->stall_sampling_ = stall_sampling;
  desc->device_ = device;
  desc->device_id_ = did;
  desc->parent_device_id_ = -1;    // no parent device
  desc->parent_device_ = nullptr;
  desc->subdevice_id_ = -1;        // not a subdevice
  desc->num_sub_devices_ = GetSubDeviceCount(device);
  desc->driver_ = driver;
  desc->context_ = context;
  desc->metric_group_ = l0_metric::GetMetricGroup(device, metric_group);
  desc->profiling_thread_ = nullptr;
  desc->profiling_state_.store(PROFILER_DISABLED, std::memory_order_release);

  return desc;
}

uint32_t
GetSubDeviceCount
(
  ze_device_handle_t device
) 
{
  uint32_t num_sub_devices = 0;
  ze_result_t status = zeDeviceGetSubDevices(device, &num_sub_devices, nullptr);
  PTI_ASSERT(status == ZE_RESULT_SUCCESS);
  return num_sub_devices;
}

void 
HandleSubDevices
(
  ZeDeviceDescriptor* parent_desc,
  std::map<ze_device_handle_t,
  ZeDeviceDescriptor*>& device_descriptors
) 
{
  uint32_t num_sub_devices = GetSubDeviceCount(parent_desc->device_);
  if (num_sub_devices == 0) return;

  std::vector<ze_device_handle_t> sub_devices(num_sub_devices);
  ze_result_t status = zeDeviceGetSubDevices(parent_desc->device_, &num_sub_devices, sub_devices.data());
  PTI_ASSERT(status == ZE_RESULT_SUCCESS);

  for (uint32_t j = 0; j < num_sub_devices; j++) {
    ZeDeviceDescriptor* sub_desc = new ZeDeviceDescriptor;
    sub_desc->stall_sampling_ = parent_desc->stall_sampling_;
    sub_desc->device_ = sub_devices[j];
    sub_desc->device_id_ = parent_desc->device_id_;
    sub_desc->parent_device_id_ = parent_desc->device_id_;
    sub_desc->parent_device_ = parent_desc->device_;
    sub_desc->subdevice_id_ = j;
    sub_desc->num_sub_devices_ = 0;
    sub_desc->driver_ = parent_desc->driver_;
    sub_desc->context_ = parent_desc->context_;
    sub_desc->metric_group_ = parent_desc->metric_group_;
    sub_desc->profiling_thread_ = nullptr;
    sub_desc->profiling_state_.store(PROFILER_DISABLED, std::memory_order_release);

    device_descriptors.insert({sub_devices[j], sub_desc});
  }
}

void
EnumerateDevices
(
  std::map<ze_device_handle_t, ZeDeviceDescriptor*>& device_descriptors, 
  std::vector<ze_context_handle_t>& metric_contexts
) 
{
  std::string metric_group = "EuStallSampling";
  bool stall_sampling = (metric_group == "EuStallSampling");

  uint32_t num_drivers = 0;
  ze_result_t status = zeDriverGet(&num_drivers, nullptr);
  PTI_ASSERT(status == ZE_RESULT_SUCCESS);

  if (num_drivers == 0) {
    return;
  }

  std::vector<ze_driver_handle_t> drivers(num_drivers);
  status = zeDriverGet(&num_drivers, drivers.data());
  PTI_ASSERT(status == ZE_RESULT_SUCCESS);

  int32_t did = 0;
  for (auto driver : drivers) {
    ze_context_handle_t context = nullptr;
    ze_context_desc_t cdesc = {ZE_STRUCTURE_TYPE_CONTEXT_DESC, nullptr, 0};
    status = zeContextCreate(driver, &cdesc, &context);
    PTI_ASSERT(status == ZE_RESULT_SUCCESS);
    metric_contexts.push_back(context);

    uint32_t num_devices = 0;
    status = zeDeviceGet(driver, &num_devices, nullptr);
    PTI_ASSERT(status == ZE_RESULT_SUCCESS);
    if (num_devices == 0) continue;

    std::vector<ze_device_handle_t> devices(num_devices);
    status = zeDeviceGet(driver, &num_devices, devices.data());
    PTI_ASSERT(status == ZE_RESULT_SUCCESS);

    for (auto device : devices) {
      ZeDeviceDescriptor* desc = CreateDeviceDescriptor(device, did, driver, context, stall_sampling, metric_group);
      if (desc == nullptr) {
        continue;
      }

      device_descriptors.insert({device, desc});
      HandleSubDevices(desc, device_descriptors);
      did++;
    }
  }
}

int
GetDeviceId
(
  const std::map<ze_device_handle_t,
  ZeDeviceDescriptor*>& device_descriptors,
  ze_device_handle_t sub_device
) 
{
  auto it = device_descriptors.find(sub_device);
  if (it != device_descriptors.end()) {
    return it->second->device_id_;
  }
  return -1;
}

int
GetSubDeviceId
(
  const std::map<ze_device_handle_t,
  ZeDeviceDescriptor*>& device_descriptors,
  ze_device_handle_t sub_device
) 
{
  auto it = device_descriptors.find(sub_device);
  if (it != device_descriptors.end()) {
    return it->second->subdevice_id_;
  }
  return -1;
}

ze_device_handle_t
GetParentDevice
(
  const std::map<ze_device_handle_t, ZeDeviceDescriptor*>& device_descriptors,
  ze_device_handle_t sub_device
) 
{
  auto it = device_descriptors.find(sub_device);
  if (it != device_descriptors.end()) {
    return it->second->parent_device_;
  }
  return nullptr;
}

}  // namespace level0_device