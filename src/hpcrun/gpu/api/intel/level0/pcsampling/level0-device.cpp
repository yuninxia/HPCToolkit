// SPDX-FileCopyrightText: 2002-2024 Rice University
// SPDX-FileCopyrightText: 2024 Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: BSD-3-Clause

// -*-Mode: C++;-*-

//*****************************************************************************
// local includes
//*****************************************************************************

#include "level0-device.hpp"


//******************************************************************************
// private operations
//******************************************************************************

static ZeDeviceDescriptor*
createDeviceDescriptor
(
  ze_device_handle_t device, 
  int32_t did, 
  ze_driver_handle_t driver, 
  ze_context_handle_t context, 
  bool stall_sampling, 
  const std::string& metric_group
) 
{
  auto desc = std::make_unique<ZeDeviceDescriptor>();

  desc->stall_sampling_ = stall_sampling;
  desc->device_ = device;
  desc->device_id_ = did;
  desc->parent_device_id_ = -1;    // no parent device
  desc->parent_device_ = nullptr;
  desc->subdevice_id_ = -1;        // not a subdevice
  desc->num_sub_devices_ = zeroGetSubDeviceCount(device);
  desc->driver_ = driver;
  desc->context_ = context;
  desc->correlation_id_ = 0;
  desc->last_correlation_id_ = 0;
  
  zeroGetMetricGroup(device, metric_group, desc->metric_group_);
  
  desc->profiling_thread_ = nullptr;
  desc->profiling_state_.store(PROFILER_DISABLED, std::memory_order_release);
  desc->running_kernel_ = nullptr;

  ze_event_pool_handle_t event_pool = zeroCreateEventPool(context, device, 2, ZE_EVENT_POOL_FLAG_HOST_VISIBLE);
  desc->serial_kernel_start_ = zeroCreateEvent(event_pool, 0, ZE_EVENT_SCOPE_FLAG_HOST, ZE_EVENT_SCOPE_FLAG_HOST);
  desc->serial_data_ready_ = zeroCreateEvent(event_pool, 1, ZE_EVENT_SCOPE_FLAG_HOST, ZE_EVENT_SCOPE_FLAG_HOST);
  desc->serial_kernel_end_ = nullptr;

  return desc.release();
}

static void 
handleSubDevices
(
  ZeDeviceDescriptor* parent_desc,
  std::map<ze_device_handle_t,
  ZeDeviceDescriptor*>& device_descriptors
) 
{
  uint32_t num_sub_devices = zeroGetSubDeviceCount(parent_desc->device_);
  if (num_sub_devices == 0) return;

  std::vector<ze_device_handle_t> sub_devices(num_sub_devices);
  ze_result_t status = zeDeviceGetSubDevices(parent_desc->device_, &num_sub_devices, sub_devices.data());
  level0_check_result(status, __LINE__);

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


//******************************************************************************
// interface operations
//******************************************************************************

std::vector<ze_device_handle_t>
zeroGetDevices
(
  ze_driver_handle_t driver
)
{
  uint32_t num_devices = 0;
  ze_result_t status = zeDeviceGet(driver, &num_devices, nullptr);
  level0_check_result(status, __LINE__);
  
  if (num_devices == 0) {
    return {};
  }
  
  std::vector<ze_device_handle_t> devices(num_devices);
  status = zeDeviceGet(driver, &num_devices, devices.data());
  level0_check_result(status, __LINE__);
  
  return devices;
}

std::vector<ze_device_handle_t>
zeroGetSubDevices
(
  ze_device_handle_t device,
  uint32_t num_sub_devices
)
{
  std::vector<ze_device_handle_t> sub_devices(num_sub_devices);
  ze_result_t status = zeDeviceGetSubDevices(device, &num_sub_devices, sub_devices.data());
  level0_check_result(status, __LINE__);
  return sub_devices;
}

uint32_t
zeroGetSubDeviceCount
(
  ze_device_handle_t device
)
{
  uint32_t num_sub_devices = 0;
  ze_result_t status = zeDeviceGetSubDevices(device, &num_sub_devices, nullptr);
  level0_check_result(status, __LINE__);
  return num_sub_devices;
}

void
zeroEnumerateDevices
(
  std::map<ze_device_handle_t, ZeDeviceDescriptor*>& device_descriptors,
  std::vector<ze_context_handle_t>& metric_contexts
)
{
  const std::string metric_group = "EuStallSampling";
  const bool stall_sampling = (metric_group == "EuStallSampling");

  std::vector<ze_driver_handle_t> drivers = zeroGetDrivers();

  int32_t did = 0;
  for (const auto& driver : drivers) {
    ze_context_handle_t context = zeroCreateContext(driver);
    metric_contexts.push_back(context);

    std::vector<ze_device_handle_t> devices = zeroGetDevices(driver);

    for (const auto& device : devices) {
      ZeDeviceDescriptor* desc = createDeviceDescriptor(device, did, driver, context, stall_sampling, metric_group);
      if (desc != nullptr) {
        device_descriptors.emplace(device, desc);
        handleSubDevices(desc, device_descriptors);
        ++did;
      }
    }
  }
}

ze_device_handle_t
zeroConvertToRootDevice
(
  ze_device_handle_t device
)
{
  ze_device_handle_t rootDevice = nullptr;
  ze_result_t status = zeDeviceGetRootDevice(device, &rootDevice);
  level0_check_result(status, __LINE__);
  return (rootDevice != nullptr) ? rootDevice : device;
}
