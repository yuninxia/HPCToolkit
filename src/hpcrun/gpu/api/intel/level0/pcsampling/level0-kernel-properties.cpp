// SPDX-FileCopyrightText: 2002-2024 Rice University
// SPDX-FileCopyrightText: 2024 Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: BSD-3-Clause

// -*-Mode: C++;-*-

//*****************************************************************************
// local includes
//*****************************************************************************

#include "level0-kernel-properties.h"


//******************************************************************************
// interface operations
//******************************************************************************

void
zeroReadKernelProperties
(
  const int32_t device_id,
  const std::string& data_dir_name,
  std::map<uint64_t, KernelProperties>& kprops
)
{
  kprops.clear();
  // kernel properties file path: <data_dir>/.kprops.<device_id>.<pid>.txt
  std::string kprops_filename = ".kprops." + std::to_string(device_id) + "." + std::to_string(getpid()) + ".txt";
  
  // Find and process the kernel property file for the current process
  for (const auto& entry : std::filesystem::directory_iterator(data_dir_name)) {
    if (entry.path().filename() == kprops_filename) {
      std::ifstream kpf(entry.path());
      if (!kpf.is_open()) {
        continue;
      }

      while (!kpf.eof()) {
        KernelProperties props;
        std::string line;

        std::getline(kpf, props.name);
        if (kpf.eof()) break;

        std::getline(kpf, line);
        if (kpf.eof()) break;
        props.base_address = std::strtoul(line.c_str(), nullptr, 0);

        std::getline(kpf, props.kernel_id);
        std::getline(kpf, props.module_id);

        std::getline(kpf, line);
        props.size = std::strtoul(line.c_str(), nullptr, 0);

        kprops.insert({props.base_address, std::move(props)});
      }
      kpf.close();
      break;  // We've found and processed the file, no need to continue searching
    }
  }
}
