// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: BSD-3-Clause

// -*-Mode: C++;-*-

#ifndef LEVEL0_TRACING_CALLBACK_METHODS_H
#define LEVEL0_TRACING_CALLBACK_METHODS_H

//*****************************************************************************
// level zero includes
//*****************************************************************************

#include <level_zero/ze_api.h>


//*****************************************************************************
// system includes
//*****************************************************************************

#include <map>
#include <shared_mutex>


//*****************************************************************************
// local includes
//*****************************************************************************

#include "../level0-id-map.h"
#include "level0-cmdlist-device-map.hpp"
#include "level0-device.hpp"
#include "level0-kernel-properties.hpp"
#include "level0-kernel-size-map.hpp"
#include "level0-module.hpp"
#include "level0-timestamp.hpp"
#include "level0-unique-id.hpp"


//******************************************************************************
// global variables
//******************************************************************************

extern std::shared_mutex modules_on_devices_mutex_;
extern std::map<ze_module_handle_t, ZeModule> modules_on_devices_;


//******************************************************************************
// struct definition
//******************************************************************************

#ifndef ZEX_STRUCTURE_KERNEL_REGISTER_FILE_SIZE_EXP
#define ZEX_STRUCTURE_KERNEL_REGISTER_FILE_SIZE_EXP (ze_structure_type_t)0x00030012
typedef struct _zex_kernel_register_file_size_exp_t {
  ze_structure_type_t stype = ZEX_STRUCTURE_KERNEL_REGISTER_FILE_SIZE_EXP;
  const void *pNext = nullptr;
  uint32_t registerFileSize;
} zex_kernel_register_file_size_exp_t;
#endif


//******************************************************************************
// interface operations
//******************************************************************************

void
OnExitModuleCreate
(
  ze_module_create_params_t* params,
  ze_result_t result,
  const struct hpcrun_foil_appdispatch_level0* dispatch
);

void
OnEnterModuleDestroy
(
  ze_module_destroy_params_t* params
);

void
OnExitKernelCreate
(
  ze_kernel_create_params_t *params,
  ze_result_t result,
  const struct hpcrun_foil_appdispatch_level0* dispatch
);

void
OnEnterCommandListAppendLaunchKernel
(
  ze_command_list_append_launch_kernel_params_t* params,
  const struct hpcrun_foil_appdispatch_level0* dispatch
);

void
OnExitCommandListAppendLaunchKernel
(
  ze_command_list_append_launch_kernel_params_t* params,
  const struct hpcrun_foil_appdispatch_level0* dispatch
);

void
OnExitCommandListCreateImmediate
(
  ze_command_list_create_immediate_params_t* params,
  void* global_user_data
);


#endif // LEVEL0_TRACING_CALLBACK_METHODS_H