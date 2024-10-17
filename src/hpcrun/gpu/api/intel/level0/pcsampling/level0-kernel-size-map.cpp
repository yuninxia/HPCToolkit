// SPDX-FileCopyrightText: 2002-2024 Rice University
// SPDX-FileCopyrightText: 2024 Contributors to the HPCToolkit Project
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

static std::unordered_map<std::string, size_t> kernel_size_map_;


//******************************************************************************
// interface operations
//******************************************************************************

void
zeroFillKernelSizeMap
(
  zebin_id_map_entry_t *entry
)
{
  SymbolVector* symbols = entry->elf_vector;
  if (symbols) {
    for (int i = 0; i < symbols->nsymbols; ++i) {
      kernel_size_map_[symbols->symbolName[i]] = symbols->symbolSize[i];
    }
  }
}

size_t
zeroGetKernelSize
(
  std::string& kernel_name
)
{
  if (!kernel_name.empty() && kernel_name.back() == '\0') {
    kernel_name.pop_back();
  }
  auto it = kernel_size_map_.find(kernel_name);
  if (it != kernel_size_map_.end()) {
    return it->second;
  }
  return -1;
}
