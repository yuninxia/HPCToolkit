// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: BSD-3-Clause

// -*-Mode: C++;-*-

#ifndef LEVEL0_TRACING_CALLBACKS_H_
#define LEVEL0_TRACING_CALLBACKS_H_

//*****************************************************************************
// level zero includes
//*****************************************************************************

#include <level_zero/layers/zel_tracing_api.h>
#include <level_zero/ze_api.h>
#include <level_zero/zet_api.h>


//******************************************************************************
// local includes
//******************************************************************************

#include "level0-collector.hpp"


//*****************************************************************************
// callback functions
//*****************************************************************************

void
zeModuleCreateOnExit
(
  ze_module_create_params_t* params,
  ze_result_t result,
  void* global_user_data,
  void** instance_user_data
);

void
zeModuleDestroyOnEnter
(
  ze_module_destroy_params_t* params,
  ze_result_t result,
  void* global_user_data,
  void** instance_user_data
);

void
zeKernelCreateOnExit
(
  ze_kernel_create_params_t* params,
  ze_result_t result,
  void* global_user_data,
  void** instance_user_data
);

void
zeCommandListAppendLaunchKernelOnEnter
(
  ze_command_list_append_launch_kernel_params_t* params,
  ze_result_t result,
  void* global_user_data,
  void** instance_user_data
);

void
zeCommandListAppendLaunchKernelOnExit
(
  ze_command_list_append_launch_kernel_params_t* params,
  ze_result_t result,
  void* global_user_data,
  void** instance_user_data
);

void
zeCommandListCreateImmediateOnExit
(
  ze_command_list_create_immediate_params_t* params,
  ze_result_t result,
  void* global_user_data,
  void** instance_user_data
);


#endif // LEVEL0_TRACING_CALLBACKS_H_
