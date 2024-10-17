// SPDX-FileCopyrightText: 2002-2024 Rice University
// SPDX-FileCopyrightText: 2024 Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: BSD-3-Clause

// -*-Mode: C++;-*-

//*****************************************************************************
// local includes
//*****************************************************************************

#include "level0-cmdlist-device-map.hpp"


//*****************************************************************************
// global variables
//*****************************************************************************

std::map<ze_device_handle_t, ZeDeviceDescriptor*> device_descriptors_;


//*****************************************************************************
// local variables
//*****************************************************************************

static std::mutex cmdlist_device_map_mutex_;
static std::map<ze_command_list_handle_t, ze_device_handle_t> cmdlist_device_map_;


//******************************************************************************
// interface operations
//******************************************************************************

void
zeroGetDeviceDesc
(
  std::map<ze_device_handle_t, ZeDeviceDescriptor*>& out_descriptors
)
{
  out_descriptors = device_descriptors_;
}

void
zeroInsertCmdListDeviceMap
(
  ze_command_list_handle_t cmdList,
  ze_device_handle_t device
)
{
  std::lock_guard<std::mutex> lock(cmdlist_device_map_mutex_);
  cmdlist_device_map_[cmdList] = device;
}

ze_device_handle_t
zeroGetDeviceForCmdList
(
  ze_command_list_handle_t cmdList
)
{
  std::lock_guard<std::mutex> lock(cmdlist_device_map_mutex_);
  auto it = cmdlist_device_map_.find(cmdList);
  if (it != cmdlist_device_map_.end()) {
    return it->second;
  }
  return nullptr;
}
