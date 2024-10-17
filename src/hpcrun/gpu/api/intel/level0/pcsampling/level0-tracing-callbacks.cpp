// SPDX-FileCopyrightText: 2002-2024 Rice University
// SPDX-FileCopyrightText: 2024 Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: BSD-3-Clause

// -*-Mode: C++;-*-

//******************************************************************************
// local includes
//******************************************************************************

#include "level0-tracing-callbacks.hpp"
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
)
{
  OnExitModuleCreate(params, result);
}

void
zeModuleDestroyOnEnter
(
  ze_module_destroy_params_t* params, 
  ze_result_t result, 
  void* global_user_data, 
  void** instance_user_data
)
{
  OnEnterModuleDestroy(params);
}

void
zeKernelCreateOnExit
(
  ze_kernel_create_params_t* params, 
  ze_result_t result, 
  void* global_user_data, 
  void** instance_user_data
)
{
  OnExitKernelCreate(params, result);
  
  ZeCollector* collector = static_cast<ZeCollector*>(global_user_data);
  zeroDumpKernelProfiles(collector->GetDataDir());
}

void
zeCommandListAppendLaunchKernelOnEnter
(
  ze_command_list_append_launch_kernel_params_t* params,
  ze_result_t result,
  void* global_user_data,
  void** instance_user_data
)
{
  OnEnterCommandListAppendLaunchKernel(params);
}

void
zeCommandListAppendLaunchKernelOnExit
(
  ze_command_list_append_launch_kernel_params_t* params,
  ze_result_t result,
  void* global_user_data,
  void** instance_user_data
)
{
  OnExitCommandListAppendLaunchKernel(params);
}

void
zeCommandListCreateImmediateOnExit
(
  ze_command_list_create_immediate_params_t* params,
  ze_result_t result,
  void* global_user_data,
  void** instance_user_data
)
{
  OnExitCommandListCreateImmediate(params, global_user_data);
}
