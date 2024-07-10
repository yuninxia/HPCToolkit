//==============================================================
// Copyright (C) Intel Corporation
//
// SPDX-License-Identifier: MIT
// =============================================================

#ifndef PTI_TOOLS_UNITRACE_LEVEL_ZERO_COLLECTOR_H_
#define PTI_TOOLS_UNITRACE_LEVEL_ZERO_COLLECTOR_H_

#include <level_zero/layers/zel_tracing_api.h>
#include <level_zero/ze_api.h>
#include <level_zero/zet_api.h>

#include <fstream>
#include <iostream>
#include <map>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <unistd.h>
#include <unordered_map>
#include <vector>

#include "../../../../../../common/lean/crypto-hash.h"
#include "../level0-id-map.h"
#include "level0-driver.h"
#include "level0-metric-profiler.h"
#include "level0-sync.h"
#include "pti_assert.h"

struct ZeKernelGroupSize {
  uint32_t x;
  uint32_t y;
  uint32_t z;
};

struct ZeKernelCommandProperties {
  std::string id_;		// unique identidier
  uint64_t size_;	// kernel binary size
  uint64_t base_addr_;	// kernel base address
  ze_device_handle_t device_;
  int32_t device_id_;
  uint32_t simd_width_;	// SIMD
  uint32_t nargs_;	// number of kernel arguments
  uint32_t nsubgrps_;	// maximal number of subgroups
  uint32_t slmsize_;	// SLM size
  uint32_t private_mem_size_;	// private memory size for each thread
  uint32_t spill_mem_size_;	// spill memory size for each thread
  ZeKernelGroupSize group_size_;	// group size
  uint32_t regsize_;	// GRF size per thread
  bool aot_;		// AOT or JIT
  std::string name_;	// kernel or command name
  std::string module_id_; // module id
  void* function_pointer_; // function pointer
};

struct ZeModule {
  ze_device_handle_t device_;
  std::string module_id_;
  size_t size_;
  bool aot_;	// AOT or JIT
  std::vector<std::string> kernel_names_; // List of kernel names
};

struct ZeDevice {
  ze_device_handle_t device_;
  ze_device_handle_t parent_device_;
  ze_driver_handle_t driver_;
  int32_t id_;
  int32_t parent_id_;
  int32_t subdevice_id_;
  int32_t num_subdevices_;
};

// these will not go away when ZeCollector is destructed
static std::shared_mutex kernel_command_properties_mutex_;
static std::map<std::string, ZeKernelCommandProperties> *kernel_command_properties_ = nullptr;
static std::shared_mutex modules_on_devices_mutex_;
static std::map<ze_module_handle_t, ZeModule> modules_on_devices_; //module to ZeModule map
// these will no go away when ZeCollector is destructed
static std::shared_mutex devices_mutex_;
static std::map<ze_device_handle_t, ZeDevice> *devices_;
static ze_result_t (*zexKernelGetBaseAddress)(ze_kernel_handle_t hKernel, uint64_t *baseAddress) = nullptr;

extern ZeMetricProfiler* metric_profiler;

#if !defined(ZEX_STRUCTURE_KERNEL_REGISTER_FILE_SIZE_EXP)

#define ZEX_STRUCTURE_KERNEL_REGISTER_FILE_SIZE_EXP (ze_structure_type_t)0x00030012
typedef struct _zex_kernel_register_file_size_exp_t {
    ze_structure_type_t stype = ZEX_STRUCTURE_KERNEL_REGISTER_FILE_SIZE_EXP; ///< [in] type of this structure
    const void *pNext = nullptr;                                             ///< [in, out][optional] pointer to extension-specific structure
    uint32_t registerFileSize;                                               ///< [out] Register file size used in kernel
} zex_kernel_register_file_size_exp_t;

#endif /* !defined(ZEX_STRUCTURE_KERNEL_REGISTER_FILE_SIZE_EXP) */

class ZeCollector {
 public:
  static ZeCollector* Create(const std::string& data_dir);
  ZeCollector(const ZeCollector& that) = delete;
  ZeCollector& operator=(const ZeCollector& that) = delete;
  ~ZeCollector();

  void DisableTracing();

 private:
  ZeCollector(const std::string& data_dir);
  void InitializeKernelCommandProperties();
  void EnumerateAndSetupDevices();
  void DumpKernelProfiles();
  void LogKernelProfiles(const ZeKernelCommandProperties* kernel, size_t size);
 
  std::string GenerateUniqueId(const uint8_t* binary_data, size_t binary_size) const;
  void FillFunctionSizeMap(zebin_id_map_entry_t *entry);
  size_t GetFunctionSize(std::string& function_name) const;

  static void OnExitModuleCreate(ze_module_create_params_t* params, ze_result_t result, void* global_data);
  static void OnEnterModuleDestroy(ze_module_destroy_params_t* params, void* global_data);
  static void OnExitKernelCreate(ze_kernel_create_params_t *params, ze_result_t result, void* global_data);
  static void zeModuleCreateOnExit(ze_module_create_params_t* params, ze_result_t result, void* global_user_data, void** instance_user_data);
  static void zeModuleDestroyOnEnter(ze_module_destroy_params_t* params, ze_result_t result, void* global_user_data, void** instance_user_data);
  static void zeKernelCreateOnExit(ze_kernel_create_params_t* params, ze_result_t result, void* global_user_data, void** instance_user_data);

  void EnableTracing(zel_tracer_handle_t tracer);

 private:
  zel_tracer_handle_t tracer_ = nullptr;
  std::string data_dir_;
  std::unordered_map<std::string, size_t> function_size_map_;
};

// Member function implementations
ZeCollector*
ZeCollector::Create
(
  const std::string& data_dir
) 
{
  ze_api_version_t version;
  zeroGetVersion(version);
  PTI_ASSERT(
    ZE_MAJOR_VERSION(version) >= 1 &&
    ZE_MINOR_VERSION(version) >= 2);

  ZeCollector* collector = new ZeCollector(data_dir);

  ze_result_t status = ZE_RESULT_SUCCESS;
  zel_tracer_desc_t tracer_desc = {
    ZEL_STRUCTURE_TYPE_TRACER_EXP_DESC, nullptr, collector};
  zel_tracer_handle_t tracer = nullptr;
  status = zelTracerCreate(&tracer_desc, &tracer);
  if (status != ZE_RESULT_SUCCESS) {
    std::cerr << "[WARNING] Unable to create Level Zero tracer" << std::endl;
    delete collector;
    return nullptr;
  }

  collector->EnableTracing(tracer);   

  collector->tracer_ = tracer;
  
  ze_driver_handle_t driver;
  uint32_t count = 1;
  if (zeDriverGet(&count, &driver) == ZE_RESULT_SUCCESS) {
    if (zeDriverGetExtensionFunctionAddress(driver, "zexKernelGetBaseAddress", (void **)&zexKernelGetBaseAddress) != ZE_RESULT_SUCCESS) {
      zexKernelGetBaseAddress = nullptr;
    }
  }

  return collector;
}

ZeCollector::ZeCollector
(
  const std::string& data_dir
) : data_dir_(data_dir) 
{
  EnumerateAndSetupDevices();
  InitializeKernelCommandProperties();
}

ZeCollector::~ZeCollector
(
  void
) 
{
  if (tracer_ != nullptr) {
    ze_result_t status = zelTracerDestroy(tracer_);
    PTI_ASSERT(status == ZE_RESULT_SUCCESS);
  }
  DumpKernelProfiles();
}

void 
ZeCollector::DisableTracing
(
  void
) 
{
  ze_result_t status = zelTracerSetEnabled(tracer_, false);
  PTI_ASSERT(status == ZE_RESULT_SUCCESS);
}

void 
ZeCollector::InitializeKernelCommandProperties
(
  void
) 
{
  kernel_command_properties_mutex_.lock();
  if (kernel_command_properties_ == nullptr) {
    kernel_command_properties_ = new std::map<std::string, ZeKernelCommandProperties>;
  }
  kernel_command_properties_mutex_.unlock();
}

void 
ZeCollector::EnumerateAndSetupDevices
(
  void
) 
{
  if (devices_ == nullptr) {
    devices_ = new std::map<ze_device_handle_t, ZeDevice>;
  }

  ze_result_t status = ZE_RESULT_SUCCESS;
  uint32_t num_drivers = 0;
  status = zeDriverGet(&num_drivers, nullptr);
  PTI_ASSERT(status == ZE_RESULT_SUCCESS);
  if (num_drivers > 0) {
    int32_t did = 0;
    std::vector<ze_driver_handle_t> drivers(num_drivers);
    status = zeDriverGet(&num_drivers, drivers.data());
    PTI_ASSERT(status == ZE_RESULT_SUCCESS);
    for (auto driver : drivers) {

      uint32_t num_devices = 0;
      status = zeDeviceGet(driver, &num_devices, nullptr);
      PTI_ASSERT(status == ZE_RESULT_SUCCESS);
      if (num_devices) {
        std::vector<ze_device_handle_t> devices(num_devices);
        status = zeDeviceGet(driver, &num_devices, devices.data());
        PTI_ASSERT(status == ZE_RESULT_SUCCESS);
        for (auto device : devices) {
          ZeDevice desc;

          desc.device_ = device;
          desc.id_ = did;
          desc.parent_id_ = -1;	// no parent
          desc.parent_device_ = nullptr;
          desc.subdevice_id_ = -1;	// not a subdevice
          desc.driver_ = driver;

          uint32_t num_sub_devices = 0;
          status = zeDeviceGetSubDevices(device, &num_sub_devices, nullptr);
          PTI_ASSERT(status == ZE_RESULT_SUCCESS);

          desc.num_subdevices_ = num_sub_devices;
          
          devices_->insert({device, std::move(desc)});

          if (num_sub_devices > 0) {
            std::vector<ze_device_handle_t> sub_devices(num_sub_devices);

            status = zeDeviceGetSubDevices(device, &num_sub_devices, sub_devices.data());
            PTI_ASSERT(status == ZE_RESULT_SUCCESS);

            for (uint32_t j = 0; j < num_sub_devices; j++) {
              ZeDevice sub_desc;

              sub_desc.device_ = sub_devices[j];
              sub_desc.parent_id_ = did;
              sub_desc.parent_device_ = device;
              sub_desc.num_subdevices_ = 0;
              sub_desc.subdevice_id_ = j;
              sub_desc.id_ = did;	// take parent device's id
              sub_desc.driver_ = driver;

              devices_->insert({sub_devices[j], std::move(sub_desc)});
            }
          }
          did++;
        }
      }
    }
  }
}

void
ZeCollector::LogKernelProfiles
(
  const ZeKernelCommandProperties* kernel, 
  size_t size
) 
{
  std::cerr << "Kernel properties:" << std::endl;
  std::cerr << "name=\"" << kernel->name_ 
            << "\", base_addr=0x" << std::hex << kernel->base_addr_
            << std::dec
            << ", size=" << size
            << ", module_id=" << kernel->module_id_ 
            << ", kernel_id=" << kernel->id_
            << std::endl;
}

void 
ZeCollector::DumpKernelProfiles
(
  void
) 
{
  kernel_command_properties_mutex_.lock();
  std::map<int32_t, std::map<uint64_t, ZeKernelCommandProperties *>> device_kprops; // sorted by device id then base address;
  for (auto it = kernel_command_properties_->begin(); it != kernel_command_properties_->end(); it++) {
    auto dkit = device_kprops.find(it->second.device_id_);
    if (dkit == device_kprops.end()) {
      std::map<uint64_t, ZeKernelCommandProperties *> kprops;
      kprops.insert({it->second.base_addr_, &(it->second)});
      device_kprops.insert({it->second.device_id_, std::move(kprops)});
    }
    else {
      if (dkit->second.find(it->second.base_addr_) != dkit->second.end()) {
        // already inserted
        continue;
      }
      dkit->second.insert({it->second.base_addr_, &(it->second)});
    }
  }

  for (auto& props : device_kprops) {
    // kernel properties file path: data_dir/.kprops.<device_id>.<pid>.txt
    std::string fpath = data_dir_ + "/.kprops."  + std::to_string(props.first) + "." + std::to_string(getpid()) + ".txt";
    std::ofstream kpfs = std::ofstream(fpath, std::ios::out | std::ios::trunc);
    uint64_t prev_base = 0;
    for (auto it = props.second.crbegin(); it != props.second.crend(); it++) {
      // quote kernel name which may contain "," 
      kpfs << "\"" << it->second->name_.c_str() << "\"" << std::endl;
      kpfs << it->second->base_addr_ << std::endl;
      kpfs << it->second->id_ << std::endl;
      kpfs << it->second->module_id_ << std::endl;

      size_t size;
      if (prev_base == 0) {
        size = it->second->size_;
      }
      else {
        size = prev_base - it->second->base_addr_;
        if (size > it->second->size_) {
          size = it->second->size_;
        }
      }
      kpfs << size << std::endl;
      prev_base = it->second->base_addr_;

      LogKernelProfiles(it->second, size);
    }
    kpfs.close();
  }

  kernel_command_properties_mutex_.unlock();
}

std::string 
ZeCollector::GenerateUniqueId
(
  const uint8_t* binary_data, 
  size_t binary_size
) const 
{
  char hash_string[CRYPTO_HASH_STRING_LENGTH] = {0};
  crypto_compute_hash_string(binary_data, binary_size, hash_string, CRYPTO_HASH_STRING_LENGTH);
  return std::string(hash_string);
}

void 
ZeCollector::FillFunctionSizeMap
(
  zebin_id_map_entry_t *entry
) 
{
  SymbolVector* symbols = entry->elf_vector;
  if (symbols) {
    for (int i = 0; i < symbols->nsymbols; ++i) {
      function_size_map_[symbols->symbolName[i]] = symbols->symbolSize[i];
    }
  }
}

size_t 
ZeCollector::GetFunctionSize
(
  std::string& function_name
) const 
{
  if (!function_name.empty() && function_name.back() == '\0') {
    function_name.pop_back();
  }
  auto it = function_size_map_.find(function_name);
  if (it != function_size_map_.end()) {
    return it->second;
  }
  return -1;
}

void 
ZeCollector::OnExitModuleCreate
(
  ze_module_create_params_t* params, 
  ze_result_t result, 
  void* global_data
) 
{
  if (result == ZE_RESULT_SUCCESS) {
    ze_module_handle_t mod = **(params->pphModule);
    ze_device_handle_t device = *(params->phDevice);

    size_t binary_size = 0;
    ze_result_t status = ZE_RESULT_SUCCESS;
    status = zetModuleGetDebugInfo(
      mod, 
      ZET_MODULE_DEBUG_INFO_FORMAT_ELF_DWARF,
      &binary_size, 
      nullptr
    );
    PTI_ASSERT(status == ZE_RESULT_SUCCESS);
    if (binary_size == 0) {
      std::cerr << "[WARNING] Unable to find kernel symbols" << std::endl;
      return;
    }

    std::vector<uint8_t> binary(binary_size);
    status = zetModuleGetDebugInfo(
      mod, 
      ZET_MODULE_DEBUG_INFO_FORMAT_ELF_DWARF,
      &binary_size, 
      binary.data()
    );
    PTI_ASSERT(status == ZE_RESULT_SUCCESS);

    ZeCollector* collector = static_cast<ZeCollector*>(global_data);
    std::string module_id = collector->GenerateUniqueId(binary.data(), binary_size);

    ZeModule m;
    
    m.device_ = device;
    m.size_ = binary_size;
    m.module_id_ = module_id;

    // Retrieve kernel names
    uint32_t kernel_count = 0;
    status = zeModuleGetKernelNames(mod, &kernel_count, nullptr);
    if (status == ZE_RESULT_SUCCESS && kernel_count > 0) {
      std::vector<const char*> kernel_names(kernel_count);
      status = zeModuleGetKernelNames(mod, &kernel_count, kernel_names.data());
      if (status == ZE_RESULT_SUCCESS) {
        for (uint32_t i = 0; i < kernel_count; ++i) {
          m.kernel_names_.emplace_back(kernel_names[i]);
        }
      } else {
        std::cerr << "[WARNING] Unable to get kernel names, status: " << status << std::endl;
      }
    } else {
      std::cerr << "[WARNING] Unable to get kernel count, status: " << status << std::endl;
    }

    modules_on_devices_mutex_.lock();
    modules_on_devices_.insert({mod, std::move(m)});
    modules_on_devices_mutex_.unlock();
  }
}

void 
ZeCollector::OnEnterModuleDestroy
(
  ze_module_destroy_params_t* params, 
  void* global_data
) 
{
  ze_module_handle_t mod = *(params->phModule);
  modules_on_devices_mutex_.lock();
  modules_on_devices_.erase(mod);
  modules_on_devices_mutex_.unlock();
}

void 
ZeCollector::OnExitKernelCreate
(
  ze_kernel_create_params_t *params, 
  ze_result_t result, 
  void* global_data
) 
{
  if (result == ZE_RESULT_SUCCESS) {
    
    ze_module_handle_t mod = *(params->phModule);
    ze_device_handle_t device = nullptr;
    // size_t module_binary_size = (size_t)(-1);
    bool aot = false;
    std::string module_id;

    modules_on_devices_mutex_.lock_shared();
    auto mit = modules_on_devices_.find(mod);
    if (mit != modules_on_devices_.end()) {
      device = mit->second.device_; 
      // module_binary_size = mit->second.size_;
      aot = mit->second.aot_;
      module_id = mit->second.module_id_;
    }
    modules_on_devices_mutex_.unlock_shared();

    // Fill function size map
    ZeCollector* collector = static_cast<ZeCollector*>(global_data);
    uint32_t zebin_id_uint32;
    sscanf(module_id.c_str(), "%8x", &zebin_id_uint32);
    zebin_id_map_entry_t *entry = zebin_id_map_lookup(zebin_id_uint32);
    if (entry != nullptr) {
      collector->FillFunctionSizeMap(entry);
    }

    int did = -1;
    if (device != nullptr) {
      devices_mutex_.lock_shared();
      auto dit = devices_->find(device);
      if (dit != devices_->end()) {
        did = dit->second.id_;
      } 
      devices_mutex_.unlock_shared();
    }
    kernel_command_properties_mutex_.lock();

    ZeKernelCommandProperties desc;

    ze_kernel_handle_t kernel = **(params->pphKernel);

    size_t name_len = 0;
    ze_result_t status = zeKernelGetName(kernel, &name_len, nullptr);
    std::vector<char> kernel_name(name_len);
    if (status == ZE_RESULT_SUCCESS && name_len > 0) {
      status = zeKernelGetName(kernel, &name_len, kernel_name.data());
    }
    desc.name_ = name_len > 0 ? std::string(kernel_name.begin(), kernel_name.end()) : "UnknownKernel";

    std::string kernel_id = collector->GenerateUniqueId(reinterpret_cast<const uint8_t*>(&kernel), sizeof(kernel));

    desc.id_ = kernel_id;
    desc.module_id_ = module_id;

    desc.aot_ = aot;
    desc.device_id_ = did;
    desc.device_ = device;

    ze_kernel_properties_t kprops{};  
    zex_kernel_register_file_size_exp_t regsize{};
    kprops.pNext = (void *)&regsize;

    status = zeKernelGetProperties(kernel, &kprops);
    PTI_ASSERT(status == ZE_RESULT_SUCCESS);
    desc.simd_width_ = kprops.maxSubgroupSize;
    desc.nargs_ = kprops.numKernelArgs;
    desc.nsubgrps_ = kprops.maxNumSubgroups;
    desc.slmsize_ = kprops.localMemSize;
    desc.private_mem_size_ = kprops.privateMemSize;
    desc.spill_mem_size_ = kprops.spillMemSize;
    ZeKernelGroupSize group_size{kprops.requiredGroupSizeX, kprops.requiredGroupSizeY, kprops.requiredGroupSizeZ};
    desc.group_size_ = group_size;
    desc.regsize_ = regsize.registerFileSize;

    // for stall sampling
    uint64_t base_addr = 0;
    // uint64_t binary_size = 0;
    if ((zexKernelGetBaseAddress != nullptr) && (zexKernelGetBaseAddress(kernel, &base_addr) == ZE_RESULT_SUCCESS)) {
      base_addr &= 0xFFFFFFFF;
      // binary_size = module_binary_size;	// store module binary size. only an upper bound is needed
    }

    desc.base_addr_ = base_addr;
    // desc.size_ = binary_size;
    desc.size_ = collector->GetFunctionSize(desc.name_);

    // Retrieve the function pointer
    void* function_pointer = nullptr;
    if (zeModuleGetFunctionPointer(mod, kernel_name.data(), &function_pointer) == ZE_RESULT_SUCCESS) {
      desc.function_pointer_ = function_pointer;
    } else {
      desc.function_pointer_ = nullptr;
      std::cerr << "[WARNING] Unable to get function pointer for kernel: " << desc.name_ << std::endl;
    }

    kernel_command_properties_->insert({desc.id_, std::move(desc)});

    kernel_command_properties_mutex_.unlock();
  }
}

void 
ZeCollector::zeModuleCreateOnExit
(
  ze_module_create_params_t* params, 
  ze_result_t result, 
  void* global_user_data, 
  void** instance_user_data
) 
{
  OnExitModuleCreate(params, result, global_user_data);
}

void
ZeCollector::zeModuleDestroyOnEnter
(
  ze_module_destroy_params_t* params, 
  ze_result_t result, 
  void* global_user_data, 
  void** instance_user_data
) 
{
  OnEnterModuleDestroy(params, global_user_data); 
}

void 
ZeCollector::zeKernelCreateOnExit
(
  ze_kernel_create_params_t* params, 
  ze_result_t result, 
  void* global_user_data, 
  void** instance_user_data
) 
{
  OnExitKernelCreate(params, result, global_user_data); 
  ZeCollector* collector = static_cast<ZeCollector*>(global_user_data);
  collector->DumpKernelProfiles();
}

static void 
OnEnterCommandListAppendLaunchKernel
(
  ze_command_list_append_launch_kernel_params_t* params,
  void* global_data, 
  void** instance_data
) 
{
  std::cerr << "OnEnterCommandListAppendLaunchKernel" << std::endl;
  
  ze_command_list_handle_t hCommandList = *(params->phCommandList);
  ze_device_handle_t hDevice;
  ze_result_t ret = zeCommandListGetDeviceHandle(hCommandList, &hDevice);
  PTI_ASSERT(ret == ZE_RESULT_SUCCESS);

  std::map<ze_device_handle_t, ZeDeviceDescriptor*> device_descriptors;
  metric_profiler->GetDeviceDescriptors(device_descriptors);
  auto it = device_descriptors.find(hDevice);
  if (it != device_descriptors.end()) {
    ZeDeviceDescriptor* desc = it->second;
    zeroNotifyKernelStart(desc);
  } else {
    std::cerr << "[Warning] Device descriptor not found for device handle: " << hDevice << std::endl;
  }
}

static void 
zeCommandListAppendLaunchKernelOnEnter
(
  ze_command_list_append_launch_kernel_params_t* params,
  ze_result_t result,
  void* global_user_data,
  void** instance_user_data
) 
{
  OnEnterCommandListAppendLaunchKernel(params, global_user_data, instance_user_data); 
}

static void 
OnExitCommandListAppendLaunchKernel
(
  ze_command_list_append_launch_kernel_params_t* params,
  ze_result_t result, 
  void* global_data, 
  void** instance_data
) 
{
  std::cerr << "OnExitCommandListAppendLaunchKernel" << std::endl;
  
  ze_command_list_handle_t hCommandList = *(params->phCommandList);
  ze_device_handle_t hDevice;
  ze_result_t ret = zeCommandListGetDeviceHandle(hCommandList, &hDevice);
  PTI_ASSERT(ret == ZE_RESULT_SUCCESS);

  std::map<ze_device_handle_t, ZeDeviceDescriptor*> device_descriptors;
  metric_profiler->GetDeviceDescriptors(device_descriptors);

  auto it = device_descriptors.find(hDevice);
  if (it != device_descriptors.end()) {
    ZeDeviceDescriptor* desc = it->second;
    zeroNotifyKernelEnd(desc);
    zeroPCSampleSync(desc);
  } else {
    std::cerr << "[Warning] Device descriptor not found for device handle: " << hDevice << std::endl;
  }
}

static void zeCommandListAppendLaunchKernelOnExit
(
  ze_command_list_append_launch_kernel_params_t* params,
  ze_result_t result,
  void* global_user_data,
  void** instance_user_data
) 
{
  OnExitCommandListAppendLaunchKernel(params, result, global_user_data, instance_user_data); 
}

void 
ZeCollector::EnableTracing
(
  zel_tracer_handle_t tracer
) 
{
  zet_core_callbacks_t prologue = {};
  zet_core_callbacks_t epilogue = {};

  epilogue.Module.pfnCreateCb = zeModuleCreateOnExit;
  prologue.Module.pfnDestroyCb = zeModuleDestroyOnEnter;
  epilogue.Kernel.pfnCreateCb = zeKernelCreateOnExit;
  prologue.CommandList.pfnAppendLaunchKernelCb = zeCommandListAppendLaunchKernelOnEnter;
  epilogue.CommandList.pfnAppendLaunchKernelCb = zeCommandListAppendLaunchKernelOnExit;

  ze_result_t status = ZE_RESULT_SUCCESS;
  status = zelTracerSetPrologues(tracer, &prologue);
  PTI_ASSERT(status == ZE_RESULT_SUCCESS);
  status = zelTracerSetEpilogues(tracer, &epilogue);
  PTI_ASSERT(status == ZE_RESULT_SUCCESS);
  status = zelTracerSetEnabled(tracer, true);
  PTI_ASSERT(status == ZE_RESULT_SUCCESS);
}

#endif // PTI_TOOLS_UNITRACE_LEVEL_ZERO_COLLECTOR_H_