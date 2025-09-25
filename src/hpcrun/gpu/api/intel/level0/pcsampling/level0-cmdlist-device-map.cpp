// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: BSD-3-Clause

// -*-Mode: C++;-*-

//*****************************************************************************
// system includes
//*****************************************************************************

#include <map>
#include <mutex>
#include <shared_mutex>


//*****************************************************************************
// local includes
//*****************************************************************************

#include "level0-cmdlist-device-map.hpp"
#include "level0-correlation-device-map.h"
#include "level0-pc-api-receiver.hpp"


//*****************************************************************************
// global variables
//*****************************************************************************

// Global mapping from device handles to their descriptors
std::map<ze_device_handle_t, ZeDeviceDescriptor*> device_descriptors_;


//*****************************************************************************
// local variables
//*****************************************************************************

// Mutex to protect access to the device descriptors (using shared_mutex for read-write lock)
static std::shared_mutex device_descriptors_mutex_;

// Mutex to protect access to the command list to device mapping
static std::mutex cmdlist_device_map_mutex_;

// Map from command list handles to device handles
static std::map<ze_command_list_handle_t, ze_device_handle_t> cmdlist_device_map_;


//******************************************************************************
// interface operations
//******************************************************************************

void
level0GetDeviceDesc
(
  std::map<ze_device_handle_t, ZeDeviceDescriptor*>& out_descriptors
)
{
  // Use shared lock for read access
  std::shared_lock<std::shared_mutex> lock(device_descriptors_mutex_);
  out_descriptors = device_descriptors_;
}

void
level0InsertCmdListDeviceMap
(
  ze_command_list_handle_t cmdList,
  ze_device_handle_t device
)
{
  if (cmdList == nullptr) {
    pcsampling::warn("Null command list handle passed to level0InsertCmdListDeviceMap");
    return;
  }

  if (device == nullptr) {
    pcsampling::warn("Null device handle passed to level0InsertCmdListDeviceMap");
    return;
  }

  std::lock_guard<std::mutex> lock(cmdlist_device_map_mutex_);
  cmdlist_device_map_[cmdList] = device;
  hpcrun_level0_cmdlist_device_register_with_device(cmdList, device);
}

ze_device_handle_t
level0GetDeviceForCmdList
(
  ze_command_list_handle_t cmdList
)
{
  if (cmdList == nullptr) {
    pcsampling::warn("Null command list handle passed to level0GetDeviceForCmdList");
    return nullptr;
  }

  std::lock_guard<std::mutex> lock(cmdlist_device_map_mutex_);
  auto it = cmdlist_device_map_.find(cmdList);
  if (it == cmdlist_device_map_.end()) {
    pcsampling::warn("No device found for command list: %p", (void*)cmdList);
    return nullptr;
  }
  return it->second;
}

void
level0InsertDeviceDescriptor
(
  ze_device_handle_t device,
  ZeDeviceDescriptor* descriptor
)
{
  if (device == nullptr) {
    pcsampling::warn("Null device handle passed to level0InsertDeviceDescriptor");
    if (descriptor != nullptr) {
      level0DestroyDeviceDescriptor(descriptor);
    }
    return;
  }

  if (descriptor == nullptr) {
    pcsampling::warn("Null descriptor passed to level0InsertDeviceDescriptor");
    return;
  }

  // Use unique lock for write access
  std::unique_lock<std::shared_mutex> lock(device_descriptors_mutex_);

  // Check if device already exists and clean up old descriptor if needed
  auto it = device_descriptors_.find(device);
  if (it != device_descriptors_.end() && it->second != nullptr) {
    hpcrun_level0_device_unregister(device);
    level0DestroyDeviceDescriptor(it->second);
  }

  device_descriptors_[device] = descriptor;
  hpcrun_level0_device_register(device, descriptor->device_id_);
}

void
level0CleanupDeviceDescriptors
(
  void
)
{
  // Use unique lock for write access
  std::unique_lock<std::shared_mutex> lock(device_descriptors_mutex_);

  // Delete all device descriptors
  for (auto& [device, descriptor] : device_descriptors_) {
    if (descriptor != nullptr) {
      hpcrun_level0_device_unregister(device);
      level0DestroyDeviceDescriptor(descriptor);
    }
  }

  // Clear the map
  device_descriptors_.clear();
}
