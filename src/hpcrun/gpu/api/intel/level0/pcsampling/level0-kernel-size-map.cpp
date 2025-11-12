// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: Apache-2.0
//
// SPDX-License-Identifier: MIT
// Copyright (C) 2024 Intel Corporation
// This file was inspired by and uses some code fragments from Intel's
// MIT-licensed pti-gpu (https://github.com/intel/pti-gpu)

// -*-Mode: C++;-*-

//*****************************************************************************
// system includes
//*****************************************************************************

#include <mutex>

//*****************************************************************************
// local includes
//*****************************************************************************

#include "level0-kernel-size-map.hpp"
#include "level0-pc-api-receiver.hpp"


//*****************************************************************************
// local variables
//*****************************************************************************

// Global map to store kernel sizes, keyed by kernel name
static std::unordered_map<std::string, size_t> kernel_size_map_;

// Mutex to protect concurrent access to kernel_size_map_
static std::mutex kernel_size_map_mutex_;


//******************************************************************************
// interface operations
//******************************************************************************

void
level0FillKernelSizeMap
(
  zebin_id_map_entry_t *entry
)
{
  if (entry == nullptr) {
    pcsampling::warn("Null entry passed to level0FillKernelSizeMap");
    return;
  }

  SymbolVector* symbols = entry->elf_vector;
  if (symbols == nullptr) {
    pcsampling::warn("Null symbol vector in entry passed to level0FillKernelSizeMap");
    return;
  }

  if (symbols->nsymbols <= 0) {
    pcsampling::warn("No symbols found in entry passed to level0FillKernelSizeMap");
    return;
  }

  // Loop through each symbol and record its size in the map
  std::lock_guard<std::mutex> lock(kernel_size_map_mutex_);
  for (int i = 0; i < symbols->nsymbols; ++i) {
    if (symbols->symbolName[i] != nullptr) {
      kernel_size_map_[symbols->symbolName[i]] = symbols->symbolSize[i];
    } else {
      pcsampling::warn("Null symbol name at index %d", i);
    }
  }
}

size_t
level0GetKernelSize
(
  const std::string &kernel_name
)
{
  if (kernel_name.empty()) {
    pcsampling::warn("Empty kernel name passed to level0GetKernelSize");
    return static_cast<size_t>(-1);
  }

  // Create a copy and remove trailing null character if present
  std::string name = kernel_name;
  if (name.back() == '\0') {
    name.pop_back();
  }

  std::lock_guard<std::mutex> lock(kernel_size_map_mutex_);
  auto it = kernel_size_map_.find(name);
  if (it != kernel_size_map_.end()) {
    return it->second;
  }

  // Log a warning if the kernel name is not found
  pcsampling::warn("Kernel size not found for kernel: %s", name.c_str());

  // Return size_t(-1) if kernel name is not found
  return static_cast<size_t>(-1);
}
