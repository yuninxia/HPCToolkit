// SPDX-FileCopyrightText: 2002-2024 Rice University
// SPDX-FileCopyrightText: 2024 Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: BSD-3-Clause

// -*-Mode: C++;-*-

#ifndef LEVEL0_KERNEL_PROPERTIES_H
#define LEVEL0_KERNEL_PROPERTIES_H

//*****************************************************************************
// level zero includes
//*****************************************************************************

#include <level_zero/ze_api.h>


//*****************************************************************************
// system includes
//*****************************************************************************

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <shared_mutex>
#include <string>
#include <unistd.h>


//*****************************************************************************
// local includes
//*****************************************************************************

#include "level0-module.hpp"


//*****************************************************************************
// type definitions
//*****************************************************************************

struct ZeKernelGroupSize {
  uint32_t x;
  uint32_t y;
  uint32_t z;
};

struct ZeKernelCommandProperties {
  std::string kernel_id_;
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
  uint64_t function_pointer_;
};

struct KernelProperties {
  std::string name;
  uint64_t base_address;
  std::string kernel_id;
  std::string module_id;
  size_t size;
  size_t sample_count;
};

struct KernelTimeInfo {
  uint64_t cid;                  // Correlation ID
  uint64_t duration_ns;          // Duration in nanoseconds
  uint64_t kernel_launch_count;  // Number of times this kernel was launched
};

struct KernelTimingData {
  std::map<uint64_t, std::vector<KernelTimeInfo>> pc_timing_map;  // PC -> vector of timing info
  std::mutex mutex;                                               // Protect concurrent access
};


//******************************************************************************
// global variables
//******************************************************************************

extern std::shared_mutex kernel_command_properties_mutex_;
extern std::map<std::string, ZeKernelCommandProperties> *kernel_command_properties_;
extern KernelTimingData kernel_timing_data_;
extern bool concurrent_metric_profiling;


//******************************************************************************
// interface operations
//******************************************************************************

void
zeroInitializeKernelCommandProperties
(
  void
);

void
zeroReadKernelProperties
(
  const int32_t device_id,
  const std::string& data_dir_name,
  std::map<uint64_t, KernelProperties>& kprops
);

void
zeroInitializeKernelBaseAddressFunction
(
  void
);

uint64_t
zeroGetKernelBaseAddress
(
  ze_kernel_handle_t kernel
);

void
zeroDumpKernelProfiles
(
  const std::string& data_dir_
);


#endif // LEVEL0_KERNEL_PROPERTIES_H