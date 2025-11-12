// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: Apache-2.0
//
// SPDX-License-Identifier: MIT
// Copyright (C) 2024 Intel Corporation
// This file was inspired by and uses some code fragments from Intel's
// MIT-licensed pti-gpu (https://github.com/intel/pti-gpu)

// -*-Mode: C++;-*-

#ifndef LEVEL0_CMDLIST_DEVICE_MAP_H_
#define LEVEL0_CMDLIST_DEVICE_MAP_H_

//*****************************************************************************
// level zero includes
//*****************************************************************************

#include <level_zero/ze_api.h>


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

void
level0InsertDeviceDescriptor
(
  ze_device_handle_t device,         // [in] device handle to be mapped
  ZeDeviceDescriptor* descriptor     // [in] device descriptor (ownership transferred)
);

void
level0CleanupDeviceDescriptors
(
  void  // Clean up all device descriptors and free memory
);


#endif // LEVEL0_CMDLIST_DEVICE_MAP_H_
