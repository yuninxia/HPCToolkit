// SPDX-FileCopyrightText: 2002-2024 Rice University
// SPDX-FileCopyrightText: 2024 Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: BSD-3-Clause

// -*-Mode: C++;-*-

#ifndef LEVEL0_MODULE_H_
#define LEVEL0_MODULE_H_

//*****************************************************************************
// level zero includes
//*****************************************************************************

#include <level_zero/ze_api.h>
#include <level_zero/zet_api.h>


//*****************************************************************************
// system includes
//*****************************************************************************

#include <iostream>
#include <string>
#include <vector>


//*****************************************************************************
// local includes
//*****************************************************************************

#include "level0-assert.hpp"
#include "level0-module.hpp"


//******************************************************************************
// interface operations
//******************************************************************************

std::string
zeroGetKernelName
(
  ze_kernel_handle_t kernel
);

void*
zeroGetFunctionPointer
(
  ze_module_handle_t module,
  const std::string& kernel_name
);

std::vector<uint8_t>
zeroGetModuleDebugInfo
(
  ze_module_handle_t module
);

std::vector<std::string>
zeroGetModuleKernelNames
(
  ze_module_handle_t module
);


#endif // LEVEL0_MODULE_H_