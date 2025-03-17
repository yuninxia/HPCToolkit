// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: BSD-3-Clause

// -*-Mode: C++;-*-

//*****************************************************************************
// local includes
//*****************************************************************************

#include "level0-tracing-callback-methods.hpp"


//*****************************************************************************
// global variables
//*****************************************************************************

std::shared_mutex modules_on_devices_mutex_;
std::map<ze_module_handle_t, ZeModule> modules_on_devices_;


//******************************************************************************
// local variables
//******************************************************************************

static std::shared_mutex devices_mutex_;


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
  if (hCommandList == nullptr) {
    std::cerr << "[ERROR] Null command list in getDeviceForCommandList" << std::endl;
    return nullptr;
  }

  ze_device_handle_t hDevice = nullptr;
#if 0
  // Option 1: Use the compute runtime (requires level0 >= v1.9.0)
  ze_result_t status = f_zeCommandListGetDeviceHandle(hCommandList, &hDevice, dispatch);
  level0_check_result(status, __LINE__);
#else
  // Option 2: Manually maintain the mapping
  hDevice = level0GetDeviceForCmdList(hCommandList);
  if (hDevice == nullptr) {
    std::cerr << "[WARNING] No device found for command list: " << hCommandList << std::endl;
    return nullptr;
  }
#endif

  // Return the root device for proper notification and synchronization
  ze_device_handle_t rootDevice = level0DeviceGetRootDevice(hDevice, dispatch);
  if (rootDevice == nullptr) {
    std::cerr << "[WARNING] Failed to get root device for device: " << hDevice << std::endl;
    return hDevice; // Return the original device as fallback
  }
  return rootDevice;
}

static ZeDeviceDescriptor*
getDeviceDescriptor
(
  ze_device_handle_t hDevice
)
{
  std::map<ze_device_handle_t, ZeDeviceDescriptor*> device_descriptors;
  level0GetDeviceDesc(device_descriptors);
  auto it = device_descriptors.find(hDevice);
  if (it == device_descriptors.end()) {
    std::cerr << "[Warning] Device descriptor not found for device handle: " << hDevice << std::endl;
    return nullptr;
  }
  return it->second;
}

static ZeModule
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
  m.module_id_ = level0GenerateUniqueId(&mod, sizeof(mod));
  m.kernel_names_ = level0GetModuleKernelNames(mod, dispatch);
  return m;
}

static ZeKernelCommandProperties
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
  desc.kernel_id_ = level0GenerateUniqueId(&kernel, sizeof(kernel));

  desc.name_ = level0GetKernelName(kernel, dispatch);
  desc.base_addr_ = level0GetKernelBaseAddress(kernel, dispatch);
  desc.size_ = level0GetKernelSize(desc.name_);
  desc.function_pointer_ = level0GetFunctionPointer(mod, desc.name_, dispatch);
  desc.device_ = device;

  // Query kernel properties
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

static void
waitForEventReady
(
  std::atomic<bool>& shared_var,
  const struct hpcrun_foil_appdispatch_level0* dispatch
)
{
  while (!shared_var.load(std::memory_order_acquire)) {
    // Yield CPU time to avoid busy-waiting
    std::this_thread::yield();
  }
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

  std::vector<uint8_t> binary = level0GetModuleDebugInfo(mod, dispatch);
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
  ze_kernel_create_params_t* params,
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
  zebin_id_map_entry_t* entry = zebin_id_map_lookup(zebin_id_uint32);
  if (entry != nullptr) {
    level0FillKernelSizeMap(entry);
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

  ZeDeviceDescriptor* desc = getDeviceDescriptor(hDevice);
  if (desc) {
    desc->running_kernel_ = hKernel;
    desc->running_kernel_end_ = hSignalEvent;
    desc->kernel_started_.store(true, std::memory_order_release);
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
  KernelExecutionTime executionTime = level0GetKernelExecutionTime(hSignalEvent, hDevice, dispatch);
  std::cout << "OnExitCommandListAppendLaunchKernel:  hKernel=" << hKernel << ", hDevice=" << hDevice
            << ", Start time: " << executionTime.startTimeNs << " ns"
            << ", End time: " << executionTime.endTimeNs << " ns"
            << ", Execution time: " << executionTime.executionTimeNs << " ns" << std::endl;
#endif

  ZeDeviceDescriptor* desc = getDeviceDescriptor(hDevice);
  if (desc) {
    // Wait until the data ready event is signaled
    waitForEventReady(desc->serial_data_ready_, dispatch);
    // Reset the event flag after processing
    desc->serial_data_ready_.store(false, std::memory_order_release);
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
  level0InsertCmdListDeviceMap(hCommandList, hDevice);
}
