// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: BSD-3-Clause

// -*-Mode: C++;-*-

//*****************************************************************************
// system includes
//*****************************************************************************

#include <iostream>
#include <mutex>

//*****************************************************************************
// local includes
//*****************************************************************************

#include "level0-kernel-size-map.hpp"
#include "pcsampling-api-receiver.hpp"


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

// Original function that tries to access opaque pointer fields
// This function cannot work when zebin_id_map_entry_t is opaque
// Keeping for backward compatibility but it should not be called directly
void
level0FillKernelSizeMap
(
  zebin_id_map_entry_t *entry
)
{
  // This function cannot work with opaque pointers
  // Use level0FillKernelSizeMapFromSymbols instead
  std::cerr << "[WARNING] level0FillKernelSizeMap called with opaque pointer - skipping" << std::endl;
  return;
}

// New function that takes symbol data directly
// This can be called from the wrapper in pcsampling-shim.c
extern "C" void
level0FillKernelSizeMapFromSymbols
(
  int nsymbols,
  const char** symbolNames,
  const size_t* symbolSizes
)
{
  if (nsymbols <= 0) {
    std::cerr << "[WARNING] No symbols provided to level0FillKernelSizeMapFromSymbols" << std::endl;
    return;
  }

  if (symbolNames == nullptr || symbolSizes == nullptr) {
    std::cerr << "[WARNING] Null symbol data passed to level0FillKernelSizeMapFromSymbols" << std::endl;
    return;
  }

  // Loop through each symbol and record its size in the map
  std::lock_guard<std::mutex> lock(kernel_size_map_mutex_);
  for (int i = 0; i < nsymbols; ++i) {
    if (symbolNames[i] != nullptr) {
      kernel_size_map_[symbolNames[i]] = symbolSizes[i];
    } else {
      std::cerr << "[WARNING] Null symbol name at index " << i << std::endl;
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
    std::cerr << "[WARNING] Empty kernel name passed to level0GetKernelSize" << std::endl;
    return static_cast<size_t>(-1);
  }

  // Use the kernel size lookup from libhpcrun through the API
  // This ensures we get the kernel size from where the symbols were actually loaded
  size_t size = pcsampling::lookupKernelSize(kernel_name.c_str());

  // Warnings disabled - kernel size lookup may not be needed for PC sampling

  return size;
}
