// SPDX-FileCopyrightText: 2002-2024 Rice University
// SPDX-FileCopyrightText: 2024 Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: BSD-3-Clause

// -*-Mode: C++;-*-

//*****************************************************************************
// local includes
//*****************************************************************************

#include "level0-tracing-callback-methods.hpp"


//******************************************************************************
// global variables
//******************************************************************************

std::shared_mutex modules_on_devices_mutex_;
std::map<ze_module_handle_t, ZeModule> modules_on_devices_;


//******************************************************************************
// local variables
//******************************************************************************

std::shared_mutex devices_mutex_;


//******************************************************************************
// private operations
//******************************************************************************

static ze_device_handle_t
getDeviceForCommandList
(
  ze_command_list_handle_t hCommandList,
  const struct hpcrun_foil_appdispatch_level0* dispatch
)
{
  ze_device_handle_t hDevice;
#if 0
  // Option 1: with compute runtime using level0 >= v1.9.0, but the binary is broken for now
  ze_result_t status = f_zeCommandListGetDeviceHandle(hCommandList, &hDevice, dispatch);
  level0_check_result(status, __LINE__);
#else
  // Option 2: manually maintain the mapping, generally more reliable
  hDevice = zeroGetDeviceForCmdList(hCommandList);
#endif

  // Use the root device for notification and synchronization
  return zeroDeviceGetRootDevice(hDevice, dispatch);
}

static ZeDeviceDescriptor*
getDeviceDescriptor
(
  ze_device_handle_t hDevice
)
{
  std::map<ze_device_handle_t, ZeDeviceDescriptor*> device_descriptors;
  zeroGetDeviceDesc(device_descriptors);
  auto it = device_descriptors.find(hDevice);
  if (it == device_descriptors.end()) {
    std::cerr << "[Warning] Device descriptor not found for device handle: " << hDevice << std::endl;
    return nullptr;
  }
  return it->second;
}

ZeModule
createZeModule
(
  ze_module_handle_t mod,
  ze_device_handle_t device,
  const std::vector<uint8_t>& binary,
  const struct hpcrun_foil_appdispatch_level0* dispatch
)
{
  ZeModule m;
  m.device_ = device;
  m.size_ = binary.size();
  m.module_id_ = zeroGenerateUniqueId(&mod, sizeof(mod));
  m.kernel_names_ = zeroGetModuleKernelNames(mod, dispatch);
  return m;
}

ZeKernelCommandProperties
extractKernelProperties
(
  ze_kernel_handle_t kernel,
  const std::string& module_id,
  ze_module_handle_t mod,
  bool aot,
  int device_id,
  ze_device_handle_t device,
  const struct hpcrun_foil_appdispatch_level0* dispatch
)
{
  ZeKernelCommandProperties desc;

  desc.device_id_ = device_id;
  desc.module_id_ = module_id;
  desc.kernel_id_ = zeroGenerateUniqueId(&kernel, sizeof(kernel));
  
  desc.name_ = zeroGetKernelName(kernel, dispatch);
  desc.base_addr_ = zeroGetKernelBaseAddress(kernel, dispatch);
  desc.size_ = zeroGetKernelSize(desc.name_);
  desc.function_pointer_ = zeroGetFunctionPointer(mod, desc.name_, dispatch);

  desc.device_ = device;
  
  ze_kernel_properties_t kprops{};  
  zex_kernel_register_file_size_exp_t regsize{};
  kprops.pNext = (void *)&regsize;
  ze_result_t status = f_zeKernelGetProperties(kernel, &kprops, dispatch);
  level0_check_result(status, __LINE__);
  desc.simd_width_ = kprops.maxSubgroupSize;
  desc.nargs_ = kprops.numKernelArgs;
  desc.nsubgrps_ = kprops.maxNumSubgroups;
  desc.slmsize_ = kprops.localMemSize;
  desc.private_mem_size_ = kprops.privateMemSize;
  desc.spill_mem_size_ = kprops.spillMemSize;
  ZeKernelGroupSize group_size{kprops.requiredGroupSizeX, kprops.requiredGroupSizeY, kprops.requiredGroupSizeZ};
  desc.group_size_ = group_size;
  desc.regsize_ = regsize.registerFileSize;
  desc.aot_ = aot;

  return desc;
}


//******************************************************************************
// interface operations
//******************************************************************************

void
OnExitModuleCreate
(
  ze_module_create_params_t* params,
  ze_result_t result,
  const struct hpcrun_foil_appdispatch_level0* dispatch
)
{
  assert(result == ZE_RESULT_SUCCESS && "Module creation failed unexpectedly");

  ze_module_handle_t mod = **(params->pphModule);
  ze_device_handle_t device = *(params->phDevice);

  std::vector<uint8_t> binary = zeroGetModuleDebugInfo(mod, dispatch);
  if (binary.empty()) {
    return;
  }

  ZeModule m = createZeModule(mod, device, binary, dispatch);

  modules_on_devices_mutex_.lock();
  modules_on_devices_.insert({mod, std::move(m)});
  modules_on_devices_mutex_.unlock();
}

void
OnEnterModuleDestroy
(
  ze_module_destroy_params_t* params
)
{
  ze_module_handle_t mod = *(params->phModule);
  modules_on_devices_mutex_.lock();
  modules_on_devices_.erase(mod);
  modules_on_devices_mutex_.unlock();
}

void
OnExitKernelCreate
(
  ze_kernel_create_params_t *params,
  ze_result_t result,
  const struct hpcrun_foil_appdispatch_level0* dispatch
)
{
  assert(result == ZE_RESULT_SUCCESS && "Kernel creation failed unexpectedly");

  ze_module_handle_t mod = *(params->phModule);
  ze_device_handle_t device = nullptr;
  bool aot = false;
  std::string module_id;

  modules_on_devices_mutex_.lock_shared();
  auto mit = modules_on_devices_.find(mod);
  if (mit != modules_on_devices_.end()) {
    device = mit->second.device_; 
    aot = mit->second.aot_;
    module_id = mit->second.module_id_;
  }
  modules_on_devices_mutex_.unlock_shared();

  // Fill function size map
  uint32_t zebin_id_uint32;
  sscanf(module_id.c_str(), "%8x", &zebin_id_uint32);
  zebin_id_map_entry_t *entry = zebin_id_map_lookup(zebin_id_uint32);
  if (entry != nullptr) {
    zeroFillKernelSizeMap(entry);
  }

  int device_id = -1;
  if (device != nullptr) {
    devices_mutex_.lock_shared();
    auto dit = devices_->find(device);
    if (dit != devices_->end()) {
      device_id = dit->second.id_;
    } 
    devices_mutex_.unlock_shared();
  }
  kernel_command_properties_mutex_.lock();

  ze_kernel_handle_t kernel = **(params->pphKernel);
  ZeKernelCommandProperties desc = extractKernelProperties(kernel, module_id, mod, aot, device_id, device, dispatch);
  
  kernel_command_properties_->insert({desc.kernel_id_, std::move(desc)});
  kernel_command_properties_mutex_.unlock();
}

void
OnEnterCommandListAppendLaunchKernel
(
  ze_command_list_append_launch_kernel_params_t* params,
  const struct hpcrun_foil_appdispatch_level0* dispatch
)
{
  ze_command_list_handle_t hCommandList = *(params->phCommandList);
  ze_kernel_handle_t hKernel = *(params->phKernel);
  ze_event_handle_t hSignalEvent = *(params->phSignalEvent);
  ze_device_handle_t hDevice = getDeviceForCommandList(hCommandList, dispatch);

#if 0
  std::cout << "OnEnterCommandListAppendLaunchKernel: hKernel=" << hKernel << ", hDevice=" << hDevice << std::endl;
#endif

  ZeDeviceDescriptor* desc = getDeviceDescriptor(hDevice);
  if (desc) {
    desc->running_kernel_ = hKernel;
    desc->serial_kernel_end_ = hSignalEvent;
    desc->kernel_started_.store(true, std::memory_order_release);
    
    // Signal event to notify kernel start
    ze_result_t status = f_zeEventHostSignal(desc->serial_kernel_start_, dispatch);
    level0_check_result(status, __LINE__);
  }
}

void
OnExitCommandListAppendLaunchKernel
(
  ze_command_list_append_launch_kernel_params_t* params,
  const struct hpcrun_foil_appdispatch_level0* dispatch
)
{
  ze_command_list_handle_t hCommandList = *(params->phCommandList);
  ze_device_handle_t hDevice = getDeviceForCommandList(hCommandList, dispatch);

#if 0
  ze_kernel_handle_t hKernel = *(params->phKernel);
  ze_event_handle_t hSignalEvent = *(params->phSignalEvent);
  KernelExecutionTime executionTime = zeroGetKernelExecutionTime(hSignalEvent, hDevice, dispatch);
  std::cout << "OnExitCommandListAppendLaunchKernel:  hKernel=" << hKernel << ", hDevice=" << hDevice
            << ", Start time: " << executionTime.startTimeNs << " ns" << ", End time: " << executionTime.endTimeNs << " ns"
            << "  Execution time: " << executionTime.executionTimeNs << " ns" << std::endl;
#endif

  ZeDeviceDescriptor* desc = getDeviceDescriptor(hDevice);
  if (desc) {
    // Host reset event to indicate kernel execution has ended
    ze_result_t status = f_zeEventHostReset(desc->serial_kernel_start_, dispatch);
    level0_check_result(status, __LINE__);

    status = f_zeEventHostSynchronize(desc->serial_data_ready_, UINT64_MAX - 1, dispatch);
    level0_check_result(status, __LINE__);
    status = f_zeEventHostReset(desc->serial_data_ready_, dispatch);
    level0_check_result(status, __LINE__);
  }
}

void
OnExitCommandListCreateImmediate
(
  ze_command_list_create_immediate_params_t* params,
  void* global_user_data
)
{
  assert(global_user_data != nullptr);
  ze_command_list_handle_t hCommandList = **(params->pphCommandList);
  ze_device_handle_t hDevice = *(params->phDevice);
  zeroInsertCmdListDeviceMap(hCommandList, hDevice);
}
