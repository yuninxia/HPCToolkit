// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
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

#include "../../../../../foil/level0.h"
#include "level0-assert.hpp"
#include "level0-module.hpp"


//******************************************************************************
// struct definition
//******************************************************************************

struct ZeModule {
  ze_device_handle_t device_;
  std::string module_id_;
  size_t size_;
  bool aot_;
  std::vector<std::string> kernel_names_;
};


//******************************************************************************
// interface operations
//******************************************************************************

std::string
level0GetKernelName
(
  ze_kernel_handle_t kernel,
  const struct hpcrun_foil_appdispatch_level0* dispatch
);

uint64_t
level0GetFunctionPointer
(
  ze_module_handle_t module,
  const std::string& kernel_name,
  const struct hpcrun_foil_appdispatch_level0* dispatch
);

std::vector<uint8_t>
level0GetModuleDebugInfo
(
  ze_module_handle_t module,
  const struct hpcrun_foil_appdispatch_level0* dispatch
);

std::vector<std::string>
level0GetModuleKernelNames
(
  ze_module_handle_t module,
  const struct hpcrun_foil_appdispatch_level0* dispatch
);


#endif // LEVEL0_MODULE_H_