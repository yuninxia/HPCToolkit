// SPDX-FileCopyrightText: 2002-2024 Rice University
// SPDX-FileCopyrightText: 2024 Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: BSD-3-Clause

// -*-Mode: C++;-*-

#ifndef LEVEL0_KERNEL_PROPERTIES_H
#define LEVEL0_KERNEL_PROPERTIES_H

//*****************************************************************************
// system includes
//*****************************************************************************

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <unistd.h>


//*****************************************************************************
// type definitions
//*****************************************************************************

struct KernelProperties {
  std::string name;
  uint64_t base_address;
  std::string kernel_id;
  std::string module_id;
  size_t size;
  size_t sample_count;
};


//******************************************************************************
// interface operations
//******************************************************************************

void
zeroReadKernelProperties
(
  const int32_t device_id,
  const std::string& data_dir_name,
  std::map<uint64_t, KernelProperties>& kprops
);


#endif // LEVEL0_KERNEL_PROPERTIES_H