// SPDX-FileCopyrightText: 2002-2024 Rice University
// SPDX-FileCopyrightText: 2024 Contributors to the HPCToolkit Project
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
// local variables
//******************************************************************************

#include "level0-collector.hpp"


//*****************************************************************************
// global variables
//*****************************************************************************

extern std::shared_mutex kernel_command_properties_mutex_;
extern std::map<std::string, ZeKernelCommandProperties> *kernel_command_properties_;
extern std::shared_mutex modules_on_devices_mutex_;
extern std::map<ze_module_handle_t, ZeModule> modules_on_devices_;
extern std::shared_mutex devices_mutex_;
extern std::map<ze_device_handle_t, ZeDevice> *devices_;
extern ze_result_t (*zexKernelGetBaseAddress)(ze_kernel_handle_t hKernel, uint64_t *baseAddress);

extern ZeMetricProfiler* metric_profiler;


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
