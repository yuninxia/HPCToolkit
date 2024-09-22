// SPDX-FileCopyrightText: 2002-2024 Rice University
// SPDX-FileCopyrightText: 2024 Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: BSD-3-Clause

// -*-Mode: C++;-*-

#ifndef LEVEL0_COLLECTOR_H_
#define LEVEL0_COLLECTOR_H_

//*****************************************************************************
// level zero includes
//*****************************************************************************

#include <level_zero/layers/zel_tracing_api.h>
#include <level_zero/ze_api.h>
#include <level_zero/zet_api.h>


//*****************************************************************************
// system includes
//*****************************************************************************

#include <map>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <vector>


//******************************************************************************
// local variables
//******************************************************************************

#include "../../../../../../common/lean/crypto-hash.h"
#include "../level0-id-map.h"
#include "level0-assert.hpp"
#include "level0-driver.hpp"
#include "level0-metric-profiler.hpp"


//******************************************************************************
// struct definition
//******************************************************************************

struct ZeKernelGroupSize {
  uint32_t x;
  uint32_t y;
  uint32_t z;
};

struct ZeKernelCommandProperties {
  std::string id_;
  uint64_t size_;
  uint64_t base_addr_;
  ze_device_handle_t device_;
  int32_t device_id_;
  uint32_t simd_width_;
  uint32_t nargs_;
  uint32_t nsubgrps_;
  uint32_t slmsize_;
  uint32_t private_mem_size_;
  uint32_t spill_mem_size_;
  ZeKernelGroupSize group_size_;
  uint32_t regsize_;
  bool aot_;
  std::string name_;
  std::string module_id_;
  void* function_pointer_;
};

struct ZeModule {
  ze_device_handle_t device_;
  std::string module_id_;
  size_t size_;
  bool aot_;
  std::vector<std::string> kernel_names_;
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

#ifndef ZEX_STRUCTURE_KERNEL_REGISTER_FILE_SIZE_EXP
#define ZEX_STRUCTURE_KERNEL_REGISTER_FILE_SIZE_EXP (ze_structure_type_t)0x00030012
typedef struct _zex_kernel_register_file_size_exp_t {
  ze_structure_type_t stype = ZEX_STRUCTURE_KERNEL_REGISTER_FILE_SIZE_EXP;
  const void *pNext = nullptr;
  uint32_t registerFileSize;
} zex_kernel_register_file_size_exp_t;
#endif


//*****************************************************************************
// class definition
//*****************************************************************************

class ZeCollector {
 public:
  static ZeCollector* Create(const std::string& data_dir);
  ZeCollector(const ZeCollector& that) = delete;
  ZeCollector& operator=(const ZeCollector& that) = delete;
  ~ZeCollector();

  void DisableTracing();
  void GetDeviceDescriptors(std::map<ze_device_handle_t, ZeDeviceDescriptor*>& out_descriptors);
  void InsertCommandListDeviceMapping(ze_command_list_handle_t cmdList, ze_device_handle_t device);
  ze_device_handle_t GetDeviceForCommandList(ze_command_list_handle_t cmdList);
  void DumpKernelProfiles();

  // Methods used by callbacks
  void OnExitModuleCreate(ze_module_create_params_t* params, ze_result_t result);
  void OnEnterModuleDestroy(ze_module_destroy_params_t* params);
  void OnExitKernelCreate(ze_kernel_create_params_t *params, ze_result_t result);

 private:
  ZeCollector(const std::string& data_dir);
  void InitializeKernelCommandProperties();
  void EnumerateAndSetupDevices();
  void LogKernelProfiles(const ZeKernelCommandProperties* kernel, size_t size);
 
  void EnableTracing(zel_tracer_handle_t tracer);
  std::string GenerateUniqueId(const uint8_t* binary_data, size_t binary_size) const;
  void FillFunctionSizeMap(zebin_id_map_entry_t *entry);
  size_t GetFunctionSize(std::string& function_name) const;

 private:
  zel_tracer_handle_t tracer_ = nullptr;
  std::string data_dir_;
  std::unordered_map<std::string, size_t> function_size_map_;
};


#endif // LEVEL0_COLLECTOR_H_