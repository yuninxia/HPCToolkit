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
// global variables
//******************************************************************************

std::map<ze_device_handle_t, ZeDevice> *devices_ = nullptr;


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
  const std::string& metric_group,
  const struct hpcrun_foil_appdispatch_level0* dispatch
) 
{
  ZeDeviceDescriptor* desc = new ZeDeviceDescriptor;

  desc->stall_sampling_ = stall_sampling;
  desc->device_ = device;
  desc->device_id_ = did;
  desc->parent_device_id_ = -1;    // no parent device
  desc->parent_device_ = nullptr;
  desc->subdevice_id_ = -1;        // not a subdevice
  desc->num_sub_devices_ = zeroGetSubDeviceCount(device, dispatch);
  desc->driver_ = driver;
  desc->context_ = context;
  desc->correlation_id_ = 0;
  desc->last_correlation_id_ = 0;
  
  zeroGetMetricGroup(device, metric_group, desc->metric_group_, dispatch);
  
  desc->profiling_thread_ = nullptr;
  desc->profiling_state_.store(PROFILER_DISABLED, std::memory_order_release);
  desc->running_kernel_ = nullptr;

  ze_event_pool_handle_t event_pool = zeroCreateEventPool(context, device, 2, ZE_EVENT_POOL_FLAG_HOST_VISIBLE, dispatch);
  desc->serial_kernel_start_ = zeroCreateEvent(event_pool, 0, ZE_EVENT_SCOPE_FLAG_HOST, ZE_EVENT_SCOPE_FLAG_HOST, dispatch);
  desc->serial_data_ready_ = zeroCreateEvent(event_pool, 1, ZE_EVENT_SCOPE_FLAG_HOST, ZE_EVENT_SCOPE_FLAG_HOST, dispatch);
  desc->serial_kernel_end_ = nullptr;

  return desc;
}

static ZeDeviceDescriptor*
createSubDeviceDescriptor
(
  const ZeDeviceDescriptor* parent_desc,
  ze_device_handle_t sub_device,
  uint32_t sub_device_id
)
{
  ZeDeviceDescriptor* sub_desc = new ZeDeviceDescriptor;
  sub_desc->stall_sampling_ = parent_desc->stall_sampling_;
  sub_desc->device_ = sub_device;
  sub_desc->device_id_ = parent_desc->device_id_;
  sub_desc->parent_device_id_ = parent_desc->device_id_;
  sub_desc->parent_device_ = parent_desc->device_;
  sub_desc->subdevice_id_ = sub_device_id;
  sub_desc->num_sub_devices_ = 0;
  sub_desc->driver_ = parent_desc->driver_;
  sub_desc->context_ = parent_desc->context_;
  sub_desc->metric_group_ = parent_desc->metric_group_;
  sub_desc->profiling_thread_ = nullptr;
  sub_desc->profiling_state_.store(PROFILER_DISABLED, std::memory_order_release);
  
  return sub_desc;
}

static void
SetupDevice
(
  ze_device_handle_t device,
  ze_driver_handle_t driver, 
  int32_t id,
  int32_t parent_id,
  ze_device_handle_t parent_device,
  int32_t subdevice_id,
  const struct hpcrun_foil_appdispatch_level0* dispatch
)
{
  ZeDevice desc;

  desc.device_ = device;
  desc.id_ = id; // take parent device's id if it is a subdevice
  desc.parent_id_ = parent_id;
  desc.parent_device_ = parent_device;
  desc.subdevice_id_ = subdevice_id;
  desc.driver_ = driver;
  desc.num_subdevices_ = (subdevice_id == -1) ? zeroGetSubDeviceCount(device, dispatch) : 0;

  devices_->insert({device, std::move(desc)});
}


//******************************************************************************
// interface operations
//******************************************************************************

std::vector<ze_device_handle_t>
zeroGetDevices
(
  ze_driver_handle_t driver,
  const struct hpcrun_foil_appdispatch_level0* dispatch
)
{
  uint32_t num_devices = 0;
  ze_result_t status = f_zeDeviceGet(driver, &num_devices, nullptr, dispatch);
  level0_check_result(status, __LINE__);
  
  if (num_devices == 0) {
    return {};
  }
  
  std::vector<ze_device_handle_t> devices(num_devices);
  status = f_zeDeviceGet(driver, &num_devices, devices.data(), dispatch);
  level0_check_result(status, __LINE__);
  
  return devices;
}

std::vector<ze_device_handle_t>
zeroGetSubDevices
(
  ze_device_handle_t device,
  uint32_t num_sub_devices,
  const struct hpcrun_foil_appdispatch_level0* dispatch
)
{
  std::vector<ze_device_handle_t> sub_devices(num_sub_devices);
  ze_result_t status = f_zeDeviceGetSubDevices(device, &num_sub_devices, sub_devices.data(), dispatch);
  level0_check_result(status, __LINE__);
  return sub_devices;
}

uint32_t
zeroGetSubDeviceCount
(
  ze_device_handle_t device,
  const struct hpcrun_foil_appdispatch_level0* dispatch
)
{
  uint32_t num_sub_devices = 0;
  ze_result_t status = f_zeDeviceGetSubDevices(device, &num_sub_devices, nullptr, dispatch);
  level0_check_result(status, __LINE__);
  return num_sub_devices;
}

void
zeroEnumerateDevices
(
  std::map<ze_device_handle_t, ZeDeviceDescriptor*>& device_descriptors,
  std::vector<ze_context_handle_t>& metric_contexts,
  const struct hpcrun_foil_appdispatch_level0* dispatch
)
{
  const std::string metric_group = "EuStallSampling";
  const bool stall_sampling = (metric_group == "EuStallSampling");

  std::vector<ze_driver_handle_t> drivers = zeroGetDrivers(dispatch);

  int32_t did = 0;
  for (const auto& driver : drivers) {
    ze_context_handle_t context = zeroCreateContext(driver, dispatch);
    metric_contexts.push_back(context);

    std::vector<ze_device_handle_t> devices = zeroGetDevices(driver, dispatch);

    for (const auto& device : devices) {
      // Create root device descriptor
      ZeDeviceDescriptor* root_desc = createDeviceDescriptor(device, did, driver, context, stall_sampling, metric_group, dispatch);
      if (root_desc != nullptr) {
        device_descriptors.insert({device, root_desc});

        // Create sub-device descriptors
        uint32_t num_sub_devices = root_desc->num_sub_devices_;
        if (num_sub_devices > 0) {
          std::vector<ze_device_handle_t> sub_devices = zeroGetSubDevices(device, num_sub_devices, dispatch);
          for (uint32_t j = 0; j < num_sub_devices; j++) {
            ZeDeviceDescriptor* sub_desc = createSubDeviceDescriptor(root_desc, sub_devices[j], j);
            device_descriptors.insert({sub_devices[j], sub_desc});
          }
        }
        ++did;
      }
    }
  }
}

ze_device_properties_t
zeroGetDeviceProperties
(
  ze_device_handle_t device,
  const struct hpcrun_foil_appdispatch_level0* dispatch
)
{
  ze_device_properties_t deviceProps = {};
  deviceProps.stype = ZE_STRUCTURE_TYPE_DEVICE_PROPERTIES;
  
  ze_result_t status = f_zeDeviceGetProperties(device, &deviceProps, dispatch);
  level0_check_result(status, __LINE__);
  
  return deviceProps;
}

ze_device_handle_t
zeroDeviceGetRootDevice
(
  ze_device_handle_t device,
  const struct hpcrun_foil_appdispatch_level0* dispatch
)
{
  ze_device_handle_t rootDevice = nullptr;
  ze_result_t status = f_zeDeviceGetRootDevice(device, &rootDevice, dispatch);
  level0_check_result(status, __LINE__);
  return (rootDevice != nullptr) ? rootDevice : device;
}

void
zeroEnumerateAndSetupDevices
(
  const struct hpcrun_foil_appdispatch_level0* dispatch
)
{
  if (devices_ == nullptr) {
    devices_ = new std::map<ze_device_handle_t, ZeDevice>;
  }

  std::vector<ze_driver_handle_t> drivers = zeroGetDrivers(dispatch);

  int32_t did = 0;
  for (auto driver : drivers) {
    std::vector<ze_device_handle_t> devices = zeroGetDevices(driver, dispatch);
    
    for (auto device : devices) {
      SetupDevice(device, driver, did, -1, nullptr, -1, dispatch);
      
      uint32_t num_sub_devices = zeroGetSubDeviceCount(device, dispatch);
      if (num_sub_devices > 0) {
        std::vector<ze_device_handle_t> sub_devices = zeroGetSubDevices(device, num_sub_devices, dispatch);
        for (uint32_t j = 0; j < num_sub_devices; j++) {
          SetupDevice(sub_devices[j], driver, did, did, device, j, dispatch);
        }
      }
      did++;
    }
  }
}
