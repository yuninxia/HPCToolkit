// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: BSD-3-Clause

// -*-Mode: C++;-*-

#ifndef LEVEL0_CMDLIST_DEVICE_MAP_H_
#define LEVEL0_CMDLIST_DEVICE_MAP_H_

//*****************************************************************************
// level zero includes
//*****************************************************************************

#include <level_zero/ze_api.h>


//*****************************************************************************
// system includes
//*****************************************************************************

#include <map>
#include <mutex>


//*****************************************************************************
// local includes
//*****************************************************************************

#include "level0-device.hpp"


//*****************************************************************************
// global variables
//*****************************************************************************

extern std::map<ze_device_handle_t, ZeDeviceDescriptor*> device_descriptors_;

//******************************************************************************
// interface operations
//******************************************************************************

void
level0GetDeviceDesc
(
  std::map<ze_device_handle_t, ZeDeviceDescriptor*>& out_descriptors  // [out] map from device handle to device descriptor
);

void
level0InsertCmdListDeviceMap
(
  ze_command_list_handle_t cmdList,  // [in] command list handle to be mapped
  ze_device_handle_t device          // [in] device handle associated with the command list
);

ze_device_handle_t
level0GetDeviceForCmdList
(
  ze_command_list_handle_t cmdList  // [in] command list handle to look up the associated device
);


#endif // LEVEL0_CMDLIST_DEVICE_MAP_H_