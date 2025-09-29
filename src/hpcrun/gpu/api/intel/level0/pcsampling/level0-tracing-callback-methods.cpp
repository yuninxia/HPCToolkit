// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: BSD-3-Clause

// -*-Mode: C++;-*-

//*****************************************************************************
// system includes
//*****************************************************************************

#include <thread>


//*****************************************************************************
// local includes
//*****************************************************************************

#include "level0-tracing-callback-methods.hpp"
#include "level0-pc-api-receiver.hpp"
#include "level0-kernel-properties-cache.hpp"


//*****************************************************************************
// global variables
//*****************************************************************************

std::shared_mutex modules_on_devices_mutex_;
std::map<ze_module_handle_t, ZeModule> modules_on_devices_;


//******************************************************************************
// local variables
//******************************************************************************

/*
 * NOTE: devices_mutex_ has been removed as it was redundant.
 * The devices_ map is initialized once during startup (single-threaded)
 * and only accessed for read operations afterward. std::map::find() is
 * thread-safe for concurrent read-only access when the map is not modified.
 */

/* 
 * Thread-local storage for the MCS lock node. Each application thread
 * calling the kernel launch callbacks gets its own unique node instance,
 * which is necessary for the MCS queuing lock algorithm.
 */
static thread_local mcs_node_t pc_monitoring_node;


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
    pcsampling::error("Null command list in getDeviceForCommandList");
    return nullptr;
  }

  ze_device_handle_t hDevice = nullptr;
#if 0
  // Option 1: Use the compute runtime (requires level0 >= v1.9.0)
  ze_result_t status = pcsampling::callZeCommandListGetDeviceHandle(hCommandList, &hDevice, dispatch);
  level0_check_result(status, __LINE__);
#else
  // Option 2: Manually maintain the mapping
  hDevice = level0GetDeviceForCmdList(hCommandList);
  if (hDevice == nullptr) {
    pcsampling::warn("No device found for command list: %p", (void*)hCommandList);
    return nullptr;
  }
#endif

  // Return the root device for proper notification and synchronization
  ze_device_handle_t rootDevice = level0DeviceGetRootDevice(hDevice, dispatch);
  if (rootDevice == nullptr) {
    pcsampling::warn("Failed to get root device for device: %p", (void*)hDevice);
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
    pcsampling::warn("Device descriptor not found for device handle: %p", (void*)hDevice);
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
  ze_result_t status = pcsampling::callZeKernelGetProperties(kernel, &kprops, dispatch);
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
  if (result != ZE_RESULT_SUCCESS) {
    pcsampling::error("Module creation failed with result: %d", result);
    return;
  }

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
  if (result != ZE_RESULT_SUCCESS) {
    pcsampling::error("Kernel creation failed with result: %d", result);
    return;
  }

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
  zebin_id_map_entry_t* entry = pcsampling::lookupZebinIdMap(zebin_id_uint32);
  if (entry != nullptr) {
    pcsampling::fillKernelSizeMap(entry);
  }

  int device_id = -1;
  if (device != nullptr) {
    // No mutex needed: devices_ is only modified during initialization
    // and read-only access to std::map is thread-safe
    auto dit = devices_->find(device);
    if (dit != devices_->end()) {
      device_id = dit->second.id_;
    }
  }
  ze_kernel_handle_t kernel = **(params->pphKernel);
  ZeKernelCommandProperties desc = extractKernelProperties(kernel, module_id, mod, aot, device_id, device, dispatch);
  
  // Store directly in memory cache - no file I/O
  KernelPropertiesCache::getInstance().storeKernelProperties(desc.kernel_id_, desc);
  if (std::getenv("HPCTOOLKIT_LEVEL0_DEBUG_CACHE")) {
    std::cout << "[DEBUG] Stored kernel '" << desc.name_ << "' (id: " << desc.kernel_id_ 
              << ", base_addr: 0x" << std::hex << desc.base_addr_ << std::dec
              << ") to memory cache for device " << desc.device_id_ << std::endl;
  }
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
    pcsampling::mcsLock(&desc->kernel_launch_lock, &pc_monitoring_node);
    desc->running_kernel_ = hKernel;
    desc->running_kernel_end_ = hSignalEvent;
    desc->app_SetKernelStarted(true);
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
    waitForEventReady(desc->serial_data_ready_, dispatch);
    desc->app_SetSerialDataReady(false);
    pcsampling::mcsUnlock(&desc->kernel_launch_lock, &pc_monitoring_node);
  }
}

void
OnExitCommandListCreateImmediate
(
  ze_command_list_create_immediate_params_t* params,
  void* global_user_data
)
{
  if (global_user_data == nullptr) {
    pcsampling::error("global_user_data is null in OnExitCommandListCreateImmediate");
    return;
  }
  ze_command_list_handle_t hCommandList = **(params->pphCommandList);
  ze_device_handle_t hDevice = *(params->phDevice);
  level0InsertCmdListDeviceMap(hCommandList, hDevice);
}
