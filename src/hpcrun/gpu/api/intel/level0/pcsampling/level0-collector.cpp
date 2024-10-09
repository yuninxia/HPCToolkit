// SPDX-FileCopyrightText: 2002-2024 Rice University
// SPDX-FileCopyrightText: 2024 Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: BSD-3-Clause

// -*-Mode: C++;-*-

//*****************************************************************************
// local includes
//*****************************************************************************

#include "level0-collector.hpp"
#include "level0-tracing-callbacks.hpp"


//******************************************************************************
// local variables
//******************************************************************************

std::shared_mutex kernel_command_properties_mutex_;
std::map<std::string, ZeKernelCommandProperties> *kernel_command_properties_ = nullptr;
std::shared_mutex modules_on_devices_mutex_;
std::map<ze_module_handle_t, ZeModule> modules_on_devices_;
std::shared_mutex devices_mutex_;
std::map<ze_device_handle_t, ZeDevice> *devices_ = nullptr;

ze_result_t (*zexKernelGetBaseAddress)(ze_kernel_handle_t hKernel, uint64_t *baseAddress) = nullptr;


//******************************************************************************
// private methods
//******************************************************************************

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

  std::vector<ze_driver_handle_t> drivers = zeroGetDrivers();

  int32_t did = 0;
  for (auto driver : drivers) {
    std::vector<ze_device_handle_t> devices = zeroGetDevices(driver);
    
    for (auto device : devices) {
      ZeDevice desc;

      desc.device_ = device;
      desc.id_ = did;
      desc.parent_id_ = -1;	// no parent
      desc.parent_device_ = nullptr;
      desc.subdevice_id_ = -1;	// not a subdevice
      desc.driver_ = driver;

      uint32_t num_sub_devices = zeroGetSubDeviceCount(device);
      desc.num_subdevices_ = num_sub_devices;
      
      devices_->insert({device, std::move(desc)});

      if (num_sub_devices > 0) {
        std::vector<ze_device_handle_t> sub_devices = zeroGetSubDevices(device, num_sub_devices);

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
  epilogue.CommandList.pfnCreateImmediateCb = zeCommandListCreateImmediateOnExit;

  ze_result_t status = ZE_RESULT_SUCCESS;
  status = zelTracerSetPrologues(tracer, &prologue);
  level0_check_result(status, __LINE__);
  status = zelTracerSetEpilogues(tracer, &epilogue);
  level0_check_result(status, __LINE__);
  status = zelTracerSetEnabled(tracer, true);
  level0_check_result(status, __LINE__);
}

void
ZeCollector::LogKernelProfiles
(
  const ZeKernelCommandProperties* kernel,
  size_t size
)
{
  std::cout << "Kernel properties:" << std::endl;
  std::cout << "name=\"" << kernel->name_ 
            << "\", base_addr=0x" << std::hex << kernel->base_addr_
            << std::dec
            << ", size=" << size
            << ", device_handle=" << kernel->device_
            << ", device_id=" << kernel->device_id_
            << ", module_id=" << kernel->module_id_ 
            << ", kernel_id=" << kernel->id_
            << ", work_dim=(x=" << kernel->group_size_.x << ", y=" << kernel->group_size_.y << ", z=" << kernel->group_size_.z << ")"
            << std::endl;
}

std::string 
ZeCollector::GenerateUniqueId
(
  const void *data,
  size_t binary_size
) const
{
  char hash_string[CRYPTO_HASH_STRING_LENGTH] = {0};
  crypto_compute_hash_string(data, binary_size, hash_string, CRYPTO_HASH_STRING_LENGTH);
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


//******************************************************************************
// public methods
//******************************************************************************

ZeCollector*
ZeCollector::Create
(
  const std::string& data_dir
)
{
  ze_api_version_t version;
  zeroGetVersion(version);
  assert(ZE_MAJOR_VERSION(version) >= 1 && ZE_MINOR_VERSION(version) >= 2);

  ZeCollector* collector = new ZeCollector(data_dir);

  ze_result_t status = ZE_RESULT_SUCCESS;
  zel_tracer_desc_t tracer_desc = {ZEL_STRUCTURE_TYPE_TRACER_EXP_DESC, nullptr, collector};
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

ZeCollector::~ZeCollector()
{
  if (tracer_ != nullptr) {
    ze_result_t status = zelTracerDestroy(tracer_);
    level0_check_result(status, __LINE__);
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
  level0_check_result(status, __LINE__);
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

#if 0
      LogKernelProfiles(it->second, size);
#endif
    }
    kpfs.close();
  }

  kernel_command_properties_mutex_.unlock();
}

void
ZeCollector::OnExitModuleCreate
(
  ze_module_create_params_t* params,
  ze_result_t result
)
{
  assert(result == ZE_RESULT_SUCCESS && "Module creation failed unexpectedly");

  ze_module_handle_t mod = **(params->pphModule);
  ze_device_handle_t device = *(params->phDevice);

  std::vector<uint8_t> binary = zeroGetModuleDebugInfo(mod);
  if (binary.empty()) {
    return;
  }

  std::string module_id = GenerateUniqueId(&mod, sizeof(mod));

  ZeModule m;
  
  m.device_ = device;
  m.size_ = binary.size();
  m.module_id_ = module_id;
  m.kernel_names_ = zeroGetModuleKernelNames(mod);

  modules_on_devices_mutex_.lock();
  modules_on_devices_.insert({mod, std::move(m)});
  modules_on_devices_mutex_.unlock();
}

void
ZeCollector::OnEnterModuleDestroy
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
ZeCollector::OnExitKernelCreate
(
  ze_kernel_create_params_t *params,
  ze_result_t result
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
    FillFunctionSizeMap(entry);
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

  desc.name_ = zeroGetKernelName(kernel);

  std::string kernel_id = GenerateUniqueId(&kernel, sizeof(kernel));

  desc.id_ = kernel_id;
  desc.module_id_ = module_id;

  desc.aot_ = aot;
  desc.device_id_ = did;
  desc.device_ = device;

  ze_kernel_properties_t kprops{};  
  zex_kernel_register_file_size_exp_t regsize{};
  kprops.pNext = (void *)&regsize;

  ze_result_t status = zeKernelGetProperties(kernel, &kprops);
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

  // for stall sampling
  uint64_t base_addr = 0;
  if ((zexKernelGetBaseAddress != nullptr) && (zexKernelGetBaseAddress(kernel, &base_addr) == ZE_RESULT_SUCCESS)) {
    base_addr &= 0xFFFFFFFF;
  }

  desc.base_addr_ = base_addr;
  desc.size_ = GetFunctionSize(desc.name_);

  desc.function_pointer_ = zeroGetFunctionPointer(mod, desc.name_);

  kernel_command_properties_->insert({desc.id_, std::move(desc)});

  kernel_command_properties_mutex_.unlock();
}
