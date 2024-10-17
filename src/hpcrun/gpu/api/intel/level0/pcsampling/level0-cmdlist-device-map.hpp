// SPDX-FileCopyrightText: 2002-2024 Rice University
// SPDX-FileCopyrightText: 2024 Contributors to the HPCToolkit Project
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
zeroGetDeviceDesc
(
  std::map<ze_device_handle_t, ZeDeviceDescriptor*>& out_descriptors
);

void
zeroInsertCmdListDeviceMap
(
  ze_command_list_handle_t cmdList,
  ze_device_handle_t device
);

ze_device_handle_t
zeroGetDeviceForCmdList
(
  ze_command_list_handle_t cmdList
);


#endif // LEVEL0_CMDLIST_DEVICE_MAP_H_