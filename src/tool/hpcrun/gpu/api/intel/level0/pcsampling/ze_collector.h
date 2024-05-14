//==============================================================
// Copyright (C) Intel Corporation
//
// SPDX-License-Identifier: MIT
// =============================================================

#ifndef PTI_TOOLS_UNITRACE_LEVEL_ZERO_COLLECTOR_H_
#define PTI_TOOLS_UNITRACE_LEVEL_ZERO_COLLECTOR_H_

#include <atomic>
#include <iostream>
#include <map>
#include <mutex>
#include <shared_mutex>
#include <sstream>
#include <string>
#include <vector>
#include <dlfcn.h>

#include <level_zero/ze_api.h>
#include <level_zero/zet_api.h>
#include <level_zero/layers/zel_tracing_api.h>

#include "utils.h"
#include "ze_utils.h"
#include "../../../../../../../lib/prof-lean/crypto-hash.h"
#include "../../../../../../../lib/support-lean/hpctoolkit_demangle.h"


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
};

// these will not go away when ZeCollector is destructed
static std::shared_mutex kernel_command_properties_mutex_;
static std::map<std::string, ZeKernelCommandProperties> *kernel_command_properties_ = nullptr;

struct ZeModule {
  ze_device_handle_t device_;
  std::string module_id_;
  size_t size_;
  bool aot_;	// AOT or JIT
};

static std::shared_mutex modules_on_devices_mutex_;
static std::map<ze_module_handle_t, ZeModule> modules_on_devices_; //module to ZeModule map

struct ZeDevice {
  ze_device_handle_t device_;
  ze_device_handle_t parent_device_;
  ze_driver_handle_t driver_;
  int32_t id_;
  int32_t parent_id_;
  int32_t subdevice_id_;
  int32_t num_subdevices_;
};

// these will no go away when ZeCollector is destructed
static std::shared_mutex devices_mutex_;
static std::map<ze_device_handle_t, ZeDevice> *devices_;

ze_result_t (*zexKernelGetBaseAddress)(ze_kernel_handle_t hKernel, uint64_t *baseAddress) = nullptr;

class ZeCollector {
 public: // Interface

  static ZeCollector* Create() {
    ze_api_version_t version = utils::ze::GetVersion();
    PTI_ASSERT(
        ZE_MAJOR_VERSION(version) >= 1 &&
        ZE_MINOR_VERSION(version) >= 2);

    std::string data_dir_name = utils::GetEnv("Intel_PCSampling_DataDir");
    ZeCollector* collector = new ZeCollector(data_dir_name);

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

  ZeCollector(const ZeCollector& that) = delete;

  ZeCollector& operator=(const ZeCollector& that) = delete;

  ~ZeCollector() {
    if (tracer_ != nullptr) {
      ze_result_t status = zelTracerDestroy(tracer_);
      PTI_ASSERT(status == ZE_RESULT_SUCCESS);
    }
    DumpKernelProfiles();
  }

  void DisableTracing() {
    ze_result_t status = zelTracerSetEnabled(tracer_, false);
    PTI_ASSERT(status == ZE_RESULT_SUCCESS);
  }

 private: // Implementation

  ZeCollector(std::string& data_dir_name) {
    data_dir_name_ = data_dir_name;
    EnumerateAndSetupDevices();
    InitializeKernelCommandProperties();
  }

  void InitializeKernelCommandProperties(void) {
    kernel_command_properties_mutex_.lock();
    if (kernel_command_properties_ == nullptr) {
      kernel_command_properties_ = new std::map<std::string, ZeKernelCommandProperties>;
    }
    kernel_command_properties_mutex_.unlock();
  }

  void EnumerateAndSetupDevices() {
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

  void DumpKernelProfiles(void) {
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
      std::string fpath = data_dir_name_ + "/.kprops."  + std::to_string(props.first) + "." + std::to_string(utils::GetPid()) + ".txt";
      std::ofstream kpfs = std::ofstream(fpath, std::ios::out | std::ios::trunc);
      uint64_t prev_base = 0;
      for (auto it = props.second.crbegin(); it != props.second.crend(); it++) {
        // quote kernel name which may contain "," 
        kpfs << "\"" << hpctoolkit_demangle(it->second->name_.c_str()) << "\"" << std::endl;
        kpfs << it->second->base_addr_ << std::endl;
        if (prev_base == 0) {
          kpfs << it->second->size_ << std::endl;
        }
        else {
          size_t size = prev_base - it->second->base_addr_;
          if (size > it->second->size_) {
            size = it->second->size_;
          }
          kpfs << size << std::endl;
        }
        prev_base = it->second->base_addr_;
      }
      kpfs.close();
    }

    kernel_command_properties_mutex_.unlock();
  }

  std::string GenerateModuleId(const uint8_t* binary_data, size_t binary_size) const {
    char hash_string[CRYPTO_HASH_STRING_LENGTH] = {0};
    crypto_compute_hash_string(binary_data, binary_size, hash_string, CRYPTO_HASH_STRING_LENGTH);
    return std::string(hash_string);
  }

  std::string GenerateKernelId(const std::string& kernel_name) const {
    std::string input = kernel_name;
    char hash_string[CRYPTO_HASH_STRING_LENGTH] = {0};
    crypto_compute_hash_string(input.c_str(), input.size(), hash_string, CRYPTO_HASH_STRING_LENGTH);
    return std::string(hash_string);
  }

 private: // Callbacks

  static void OnExitModuleCreate(ze_module_create_params_t* params, ze_result_t result, void* global_data) {
    if (result == ZE_RESULT_SUCCESS) {
      ze_module_handle_t mod = **(params->pphModule);
      ze_device_handle_t device = *(params->phDevice);
      size_t binary_size;
      if (zeModuleGetNativeBinary(mod, &binary_size, nullptr) != ZE_RESULT_SUCCESS) {
        binary_size = (size_t)(-1);
      }

      std::vector<uint8_t> binary(binary_size);
      if (zeModuleGetNativeBinary(mod, &binary_size, binary.data()) != ZE_RESULT_SUCCESS) {
        binary_size = (size_t)(-1);
      }

      ZeCollector* collector = static_cast<ZeCollector*>(global_data);
      std::string module_id = collector->GenerateModuleId(binary.data(), binary_size);

      ZeModule m;
      
      m.device_ = device;
      m.size_ = binary_size;
      m.module_id_ = module_id;

      modules_on_devices_mutex_.lock();
      modules_on_devices_.insert({mod, std::move(m)});
      modules_on_devices_mutex_.unlock();
    }
  }

  static void OnEnterModuleDestroy(ze_module_destroy_params_t* params, void* global_data) {
    ze_module_handle_t mod = *(params->phModule);
    modules_on_devices_mutex_.lock();
    modules_on_devices_.erase(mod);
    modules_on_devices_mutex_.unlock();
  }

#if !defined(ZEX_STRUCTURE_KERNEL_REGISTER_FILE_SIZE_EXP)

#define ZEX_STRUCTURE_KERNEL_REGISTER_FILE_SIZE_EXP (ze_structure_type_t)0x00030012
typedef struct _zex_kernel_register_file_size_exp_t {
    ze_structure_type_t stype = ZEX_STRUCTURE_KERNEL_REGISTER_FILE_SIZE_EXP; ///< [in] type of this structure
    const void *pNext = nullptr;                                             ///< [in, out][optional] pointer to extension-specific structure
    uint32_t registerFileSize;                                               ///< [out] Register file size used in kernel
} zex_kernel_register_file_size_exp_t;

#endif /* !defined(ZEX_STRUCTURE_KERNEL_REGISTER_FILE_SIZE_EXP) */

  static void OnExitKernelCreate(ze_kernel_create_params_t *params, ze_result_t result, void* global_data) {
    if (result == ZE_RESULT_SUCCESS) {
      
      ze_module_handle_t mod = *(params->phModule);
      ze_device_handle_t device = nullptr;
      size_t module_binary_size = (size_t)(-1);
      bool aot = false;
      std::string module_id;

      modules_on_devices_mutex_.lock_shared();
      auto mit = modules_on_devices_.find(mod);
      if (mit != modules_on_devices_.end()) {
        device = mit->second.device_; 
        module_binary_size = mit->second.size_;
        aot = mit->second.aot_;
        module_id = mit->second.module_id_;
      }
      modules_on_devices_mutex_.unlock_shared();

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

      ZeCollector* collector = static_cast<ZeCollector*>(global_data);
      std::string kernel_id = collector->GenerateKernelId(desc.name_);
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
      uint64_t binary_size = 0;
      if ((zexKernelGetBaseAddress != nullptr) && (zexKernelGetBaseAddress(kernel, &base_addr) == ZE_RESULT_SUCCESS)) {
        base_addr &= 0xFFFFFFFF;
        binary_size = module_binary_size;	// store module binary size. only an upper bound is needed
      }

      desc.base_addr_ = base_addr;
      desc.size_ = binary_size;

      ZeKernelCommandProperties desc2 = desc;
      kernel_command_properties_->insert({desc2.id_, std::move(desc2)});

      kernel_command_properties_mutex_.unlock();
    }
  }

  static void zeModuleCreateOnExit(
      ze_module_create_params_t* params,
      ze_result_t result,
      void* global_user_data,
      void** instance_user_data) {
    OnExitModuleCreate(params, result, global_user_data);
  }

  static void zeModuleDestroyOnEnter(
      ze_module_destroy_params_t* params,
      ze_result_t result,
      void* global_user_data,
      void** instance_user_data) {
    OnEnterModuleDestroy(params, global_user_data); 
  } 

  static void zeKernelCreateOnExit(
      ze_kernel_create_params_t* params,
      ze_result_t result,
      void* global_user_data,
      void** instance_user_data) {
    OnExitKernelCreate(params, result, global_user_data); 
  }

  void EnableTracing(zel_tracer_handle_t tracer) {
    zet_core_callbacks_t prologue = {};
    zet_core_callbacks_t epilogue = {};

    epilogue.Module.pfnCreateCb = zeModuleCreateOnExit;
    prologue.Module.pfnDestroyCb = zeModuleDestroyOnEnter;
    epilogue.Kernel.pfnCreateCb = zeKernelCreateOnExit;

    ze_result_t status = ZE_RESULT_SUCCESS;
    status = zelTracerSetPrologues(tracer, &prologue);
    PTI_ASSERT(status == ZE_RESULT_SUCCESS);
    status = zelTracerSetEpilogues(tracer, &epilogue);
    PTI_ASSERT(status == ZE_RESULT_SUCCESS);
    status = zelTracerSetEnabled(tracer, true);
    PTI_ASSERT(status == ZE_RESULT_SUCCESS);
  }

 private: // Data
  zel_tracer_handle_t tracer_ = nullptr;
  std::string data_dir_name_;
};

#endif // PTI_TOOLS_UNITRACE_LEVEL_ZERO_COLLECTOR_H_
