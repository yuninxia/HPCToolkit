// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: BSD-3-Clause

// -*-Mode: C++;-*-

//*****************************************************************************
// local includes
//*****************************************************************************

#include "level0-kernel-size-map.hpp"


//*****************************************************************************
// local variables
//*****************************************************************************

// Global map to store kernel sizes, keyed by kernel name
static std::unordered_map<std::string, size_t> kernel_size_map_;


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
    std::cerr << "[WARNING] Null entry passed to level0FillKernelSizeMap" << std::endl;
    return;
  }
  
  SymbolVector* symbols = entry->elf_vector;
  if (symbols == nullptr) {
    std::cerr << "[WARNING] Null symbol vector in entry passed to level0FillKernelSizeMap" << std::endl;
    return;
  }

  if (symbols->nsymbols <= 0) {
    std::cerr << "[WARNING] No symbols found in entry passed to level0FillKernelSizeMap" << std::endl;
    return;
  }

  // Loop through each symbol and record its size in the map
  for (int i = 0; i < symbols->nsymbols; ++i) {
    if (symbols->symbolName[i] != nullptr) {
      kernel_size_map_[symbols->symbolName[i]] = symbols->symbolSize[i];
    } else {
      std::cerr << "[WARNING] Null symbol name at index " << i << std::endl;
    }
  }
}

size_t
level0GetKernelSize
(
  std::string &kernel_name
)
{
  if (kernel_name.empty()) {
    std::cerr << "[WARNING] Empty kernel name passed to level0GetKernelSize" << std::endl;
    return static_cast<size_t>(-1);
  }

  // Remove a trailing null character if present
  if (kernel_name.back() == '\0') {
    kernel_name.pop_back();
  }
  
  auto it = kernel_size_map_.find(kernel_name);
  if (it != kernel_size_map_.end()) {
    return it->second;
  }
  
  // Log a warning if the kernel name is not found
  std::cerr << "[WARNING] Kernel size not found for kernel: " << kernel_name << std::endl;
  
  // Return size_t(-1) if kernel name is not found
  return static_cast<size_t>(-1);
}
