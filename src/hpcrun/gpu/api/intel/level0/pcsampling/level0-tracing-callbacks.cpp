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
// global variables
//*****************************************************************************

extern ZeMetricProfiler* metric_profiler;


//******************************************************************************
// private operations
//******************************************************************************

static ze_device_handle_t
getDeviceForCommandList
(
  ze_command_list_handle_t hCommandList
)
{
  ze_device_handle_t hDevice;
#if 0
  // Option 1: with compute runtime using level0 >= v1.9.0, but the binary is broken for now
  ze_result_t status = zeCommandListGetDeviceHandle(hCommandList, &hDevice);
  level0_check_result(status, __LINE__);
#else
  // Option 2: manually maintain the mapping, generally more reliable
  hDevice = metric_profiler->GetDeviceForCommandList(hCommandList);
#endif
  return zeroConvertToRootDevice(hDevice);
}

static ZeDeviceDescriptor*
getDeviceDescriptor
(
  ze_device_handle_t hDevice
)
{
  std::map<ze_device_handle_t, ZeDeviceDescriptor*> device_descriptors;
  metric_profiler->GetDeviceDescriptors(device_descriptors);
  auto it = device_descriptors.find(hDevice);
  if (it == device_descriptors.end()) {
    std::cerr << "[Warning] Device descriptor not found for device handle: " << hDevice << std::endl;
    return nullptr;
  }
  return it->second;
}


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
  ZeCollector* collector = static_cast<ZeCollector*>(global_user_data);
  collector->OnExitModuleCreate(params, result);
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
  ZeCollector* collector = static_cast<ZeCollector*>(global_user_data);
  collector->OnEnterModuleDestroy(params);
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
  ZeCollector* collector = static_cast<ZeCollector*>(global_user_data);
  collector->OnExitKernelCreate(params, result);
  collector->DumpKernelProfiles();
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
  ze_command_list_handle_t hCommandList = *(params->phCommandList);
  ze_kernel_handle_t hKernel = *(params->phKernel);
  ze_event_handle_t hSignalEvent = *(params->phSignalEvent);
  ze_device_handle_t hDevice = getDeviceForCommandList(hCommandList);

#if 0
  std::cout << "OnEnterCommandListAppendLaunchKernel: hKernel=" << hKernel << ", hDevice=" << hDevice << std::endl;
#endif

  // Use the root device for notification and synchronization
  hDevice = zeroConvertToRootDevice(hDevice);
  ZeDeviceDescriptor* desc = getDeviceDescriptor(hDevice);
  if (desc) {
    desc->running_kernel_ = hKernel;
    desc->serial_kernel_end_ = hSignalEvent;

    desc->kernel_started_.store(true, std::memory_order_release);
    
    // Signal event to notify kernel start
    ze_result_t status = zeEventHostSignal(desc->serial_kernel_start_);
    level0_check_result(status, __LINE__);
  }
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
  ze_command_list_handle_t hCommandList = *(params->phCommandList);
  ze_device_handle_t hDevice = getDeviceForCommandList(hCommandList);

#if 0
  ze_kernel_handle_t hKernel = *(params->phKernel);
  ze_event_handle_t hSignalEvent = *(params->phSignalEvent);
  KernelExecutionTime executionTime = zeroGetKernelExecutionTime(hSignalEvent, hDevice);
  std::cout << "OnExitCommandListAppendLaunchKernel:  hKernel=" << hKernel << ", hDevice=" << hDevice
            << ", Start time: " << executionTime.startTimeNs << " ns" << ", End time: " << executionTime.endTimeNs << " ns"
            << "  Execution time: " << executionTime.executionTimeNs << " ns" << std::endl;
#endif

  // Use the root device for notification and synchronization
  hDevice = zeroConvertToRootDevice(hDevice);
  ZeDeviceDescriptor* desc = getDeviceDescriptor(hDevice);
  
  std::map<ze_device_handle_t, ZeDeviceDescriptor*> device_descriptors;
  metric_profiler->GetDeviceDescriptors(device_descriptors);
  if (desc) {
    // Host reset event to indicate kernel execution has ended
    ze_result_t status = zeEventHostReset(desc->serial_kernel_start_);
    level0_check_result(status, __LINE__);

    status = zeEventHostSynchronize(desc->serial_data_ready_, UINT64_MAX - 1);
    level0_check_result(status, __LINE__);
    status = zeEventHostReset(desc->serial_data_ready_);
    level0_check_result(status, __LINE__);
  }
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
  assert(global_user_data != nullptr);
  ze_command_list_handle_t hCommandList = **(params->pphCommandList);
  ze_device_handle_t hDevice = *(params->phDevice);
  metric_profiler->InsertCommandListDeviceMapping(hCommandList, hDevice);
}
