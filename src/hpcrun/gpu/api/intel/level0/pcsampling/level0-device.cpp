// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: BSD-3-Clause

// -*-Mode: C++;-*-

//*****************************************************************************
// system includes
//*****************************************************************************

#include <vector>
#include <new>


//*****************************************************************************
// local includes
//*****************************************************************************

#include "level0-device.hpp"
#include "level0-cmdlist-device-map.hpp"
#include "pcsampling-api-receiver.hpp"


//******************************************************************************
// global variables
//******************************************************************************

std::map<ze_device_handle_t, ZeDevice>* devices_ = nullptr;


//******************************************************************************
// private operations
//******************************************************************************

static ZeDeviceDescriptor*
allocateDeviceDescriptor()
{
  void* raw = pcsampling::allocMemory(sizeof(ZeDeviceDescriptor));
  if (!raw) {
    pcsampling::error("Failed to allocate memory for ZeDeviceDescriptor");
    return nullptr;
  }
  return new (raw) ZeDeviceDescriptor();
}

static void
initializeDescriptorCommon
(
  ZeDeviceDescriptor* desc
)
{
  desc->profiling_thread_id_ = -1;
  desc->UpdateProfilerState(PROFILER_DISABLED);
  desc->running_kernel_      = nullptr;
  desc->running_kernel_end_  = nullptr;
  desc->SetKernelStarted(false);
  desc->SetSerialDataReady(false);
  mcs_init(&desc->kernel_launch_lock);
}

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
  ZeDeviceDescriptor* desc = allocateDeviceDescriptor();
  if (!desc) return nullptr;

  try {
    desc->stall_sampling_      = stall_sampling;
    desc->device_              = device;
    desc->device_id_           = did;
    desc->parent_device_id_    = -1;
    desc->parent_device_       = nullptr;
    desc->subdevice_id_        = -1;
    desc->num_sub_devices_     = level0GetSubDeviceCount(device, dispatch);
    desc->driver_              = driver;
    desc->context_             = context;
    desc->correlation_id_      = 0;

    level0GetMetricGroup(device, metric_group, desc->metric_group_, dispatch);

    initializeDescriptorCommon(desc);

    return desc;
  } catch (const std::exception& e) {
    pcsampling::error("Exception in createDeviceDescriptor: %s", e.what());
    level0DestroyDeviceDescriptor(desc);
    return nullptr;
  }
}

static ZeDeviceDescriptor*
createSubDeviceDescriptor
(
  const ZeDeviceDescriptor* parent_desc,
  ze_device_handle_t sub_device,
  uint32_t sub_device_id
)
{
  if (parent_desc == nullptr) {
    pcsampling::error("Parent device descriptor is null");
    return nullptr;
  }

  ZeDeviceDescriptor* sub_desc = allocateDeviceDescriptor();
  if (!sub_desc) return nullptr;

  try {
    sub_desc->stall_sampling_   = parent_desc->stall_sampling_;
    sub_desc->device_           = sub_device;
    sub_desc->device_id_        = parent_desc->device_id_;
    sub_desc->parent_device_id_ = parent_desc->device_id_;
    sub_desc->parent_device_    = parent_desc->device_;
    sub_desc->subdevice_id_     = sub_device_id;
    sub_desc->num_sub_devices_  = 0;
    sub_desc->driver_           = parent_desc->driver_;
    sub_desc->context_          = parent_desc->context_;
    sub_desc->metric_group_     = parent_desc->metric_group_;

    initializeDescriptorCommon(sub_desc);

    return sub_desc;
  } catch (const std::exception& e) {
    pcsampling::error("Exception in createSubDeviceDescriptor: %s", e.what());
    level0DestroyDeviceDescriptor(sub_desc);
    return nullptr;
  }
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
  desc.device_         = device;
  desc.id_             = id; // For subdevices, this is the parent's id
  desc.parent_id_      = parent_id;
  desc.parent_device_  = parent_device;
  desc.subdevice_id_   = subdevice_id;
  desc.driver_         = driver;
  // If not a subdevice, query the number of subdevices; otherwise, set to 0
  desc.num_subdevices_ = (subdevice_id == -1) ? level0GetSubDeviceCount(device, dispatch) : 0;

  devices_->insert({device, std::move(desc)});
}


//******************************************************************************
// interface operations
//******************************************************************************

std::vector<ze_device_handle_t>
level0GetDevices
(
  ze_driver_handle_t driver,
  const struct hpcrun_foil_appdispatch_level0* dispatch
)
{
  uint32_t num_devices = 0;
  ze_result_t status = pcsampling::callZeDeviceGet(driver, &num_devices, nullptr, dispatch);
  level0_check_result(status, __LINE__);
  
  if (num_devices == 0) {
    return {};
  }
  
  std::vector<ze_device_handle_t> devices(num_devices);
  status = pcsampling::callZeDeviceGet(driver, &num_devices, devices.data(), dispatch);
  level0_check_result(status, __LINE__);
  
  return devices;
}

std::vector<ze_device_handle_t>
level0GetSubDevices
(
  ze_device_handle_t device,
  uint32_t num_sub_devices,
  const struct hpcrun_foil_appdispatch_level0* dispatch
)
{
  std::vector<ze_device_handle_t> sub_devices(num_sub_devices);
  ze_result_t status = pcsampling::callZeDeviceGetSubDevices(device, &num_sub_devices, sub_devices.data(), dispatch);
  level0_check_result(status, __LINE__);
  return sub_devices;
}

uint32_t
level0GetSubDeviceCount
(
  ze_device_handle_t device,
  const struct hpcrun_foil_appdispatch_level0* dispatch
)
{
  uint32_t num_sub_devices = 0;
  ze_result_t status = pcsampling::callZeDeviceGetSubDevices(device, &num_sub_devices, nullptr, dispatch);
  level0_check_result(status, __LINE__);
  return num_sub_devices;
}

void
level0EnumerateDevices
(
  std::map<ze_device_handle_t, ZeDeviceDescriptor*>& device_descriptors,  // Not used anymore - kept for compatibility
  std::vector<ze_context_handle_t>& metric_contexts,
  const struct hpcrun_foil_appdispatch_level0* dispatch
)
{
  const std::string metric_group = "EuStallSampling";
  const bool stall_sampling = (metric_group == "EuStallSampling");

  std::vector<ze_driver_handle_t> drivers = level0GetDrivers(dispatch);

  int32_t did = 0;
  for (const auto& driver : drivers) {
    // Create a context for the current driver
    ze_context_handle_t context = level0CreateContext(driver, dispatch);
    metric_contexts.push_back(context);

    // Retrieve devices associated with the driver
    std::vector<ze_device_handle_t> devices = level0GetDevices(driver, dispatch);

    for (const auto& device : devices) {
      // Create the root device descriptor
      ZeDeviceDescriptor* root_desc = createDeviceDescriptor(device, did, driver, context, stall_sampling, metric_group, dispatch);
      if (root_desc != nullptr) {
        level0InsertDeviceDescriptor(device, root_desc);

        // If the device has sub-devices, enumerate and create their descriptors
        uint32_t num_sub_devices = root_desc->num_sub_devices_;
        if (num_sub_devices > 0) {
          std::vector<ze_device_handle_t> sub_devices = level0GetSubDevices(device, num_sub_devices, dispatch);
          for (uint32_t j = 0; j < num_sub_devices; j++) {
            ZeDeviceDescriptor* sub_desc = createSubDeviceDescriptor(root_desc, sub_devices[j], j);
            level0InsertDeviceDescriptor(sub_devices[j], sub_desc);
          }
        }
        ++did;
      }
    }
  }
}

ze_device_properties_t
level0GetDeviceProperties
(
  ze_device_handle_t device,
  const struct hpcrun_foil_appdispatch_level0* dispatch
)
{
  ze_device_properties_t deviceProps = {};
  deviceProps.stype = ZE_STRUCTURE_TYPE_DEVICE_PROPERTIES;
  
  ze_result_t status = pcsampling::callZeDeviceGetProperties(device, &deviceProps, dispatch);
  level0_check_result(status, __LINE__);
  
  return deviceProps;
}

ze_device_handle_t
level0DeviceGetRootDevice
(
  ze_device_handle_t device,
  const struct hpcrun_foil_appdispatch_level0* dispatch
)
{
  ze_device_handle_t rootDevice = nullptr;
  ze_result_t status = pcsampling::callZeDeviceGetRootDevice(device, &rootDevice, dispatch);
  level0_check_result(status, __LINE__);
  return (rootDevice != nullptr) ? rootDevice : device;
}

// FIXME(Yuning): Separate the two mechanisms.
// Fixme(Yuning)(Optional): To use a design like rocm_agent_apply_helper for device setup.
void
level0EnumerateAndSetupDevices
(
  const struct hpcrun_foil_appdispatch_level0* dispatch
)
{
  if (devices_ == nullptr) {
    void* raw = pcsampling::allocMemory(sizeof(std::map<ze_device_handle_t, ZeDevice>));
    if (!raw) {
      pcsampling::error("Failed to allocate device map");
      return;
    }
    devices_ = new (raw) std::map<ze_device_handle_t, ZeDevice>();
  }

  std::vector<ze_driver_handle_t> drivers = level0GetDrivers(dispatch);

  int32_t did = 0;
  for (auto driver : drivers) {
    std::vector<ze_device_handle_t> devices = level0GetDevices(driver, dispatch);
    
    for (auto device : devices) {
      // Set up the root device.
      SetupDevice(device, driver, did, -1, nullptr, -1, dispatch);
      
      // Set up any sub-devices.
      uint32_t num_sub_devices = level0GetSubDeviceCount(device, dispatch);
      if (num_sub_devices > 0) {
        std::vector<ze_device_handle_t> sub_devices = level0GetSubDevices(device, num_sub_devices, dispatch);
        for (uint32_t j = 0; j < num_sub_devices; j++) {
          SetupDevice(sub_devices[j], driver, did, did, device, j, dispatch);
        }
      }
      ++did;
    }
  }
}

void
level0DestroyDeviceDescriptor
(
  ZeDeviceDescriptor* descriptor
)
{
  if (!descriptor) return;
  descriptor->~ZeDeviceDescriptor();
  pcsampling::freeMemory(descriptor);
}
