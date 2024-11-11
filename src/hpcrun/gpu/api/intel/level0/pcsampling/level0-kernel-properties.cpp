// SPDX-FileCopyrightText: 2002-2024 Rice University
// SPDX-FileCopyrightText: 2024 Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: BSD-3-Clause

// -*-Mode: C++;-*-

//*****************************************************************************
// local includes
//*****************************************************************************

#include "level0-kernel-properties.hpp"


//******************************************************************************
// global variables
//******************************************************************************

std::shared_mutex kernel_command_properties_mutex_;
std::map<std::string, ZeKernelCommandProperties> *kernel_command_properties_ = nullptr;
KernelTimingData kernel_timing_data_;
bool concurrent_metric_profiling = true;
std::map<uint64_t, cct_node_t *> cid_cct_node_; 

//*****************************************************************************
// local variables
//*****************************************************************************

static ze_result_t (*zexKernelGetBaseAddress)(ze_kernel_handle_t hKernel, uint64_t *baseAddress) = nullptr;


//******************************************************************************
// private operations
//******************************************************************************

void
zeroLogKernelProfiles
(
  const ZeKernelCommandProperties* kernel,
  size_t size
)
{
  std::cout << "Kernel properties:\n";
  std::cout << "name=\"" << kernel->name_
            << "\", base_addr=0x" << std::hex << kernel->base_addr_ << std::dec
            << ", size=" << size
            << ", device_handle=" << kernel->device_
            << ", device_id=" << kernel->device_id_
            << ", module_id=" << kernel->module_id_ 
            << ", kernel_id=" << kernel->kernel_id_
            << ", work_dim=(x=" << kernel->group_size_.x
            << ", y=" << kernel->group_size_.y
            << ", z=" << kernel->group_size_.z << ")\n\n";
}


//******************************************************************************
// interface operations
//******************************************************************************

void
zeroInitializeKernelCommandProperties
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

void
zeroInitializeKernelBaseAddressFunction
(
  void
)
{
  ze_driver_handle_t driver;
  uint32_t count = 1;
  if (zeDriverGet(&count, &driver) == ZE_RESULT_SUCCESS) {
    if (zeDriverGetExtensionFunctionAddress(driver, "zexKernelGetBaseAddress", (void **)&zexKernelGetBaseAddress) != ZE_RESULT_SUCCESS) {
      zexKernelGetBaseAddress = nullptr;
    }
  }
}

uint64_t
zeroGetKernelBaseAddress
(
  ze_kernel_handle_t kernel
)
{
  uint64_t base_addr = 0;
  if (zexKernelGetBaseAddress != nullptr && zexKernelGetBaseAddress(kernel, &base_addr) == ZE_RESULT_SUCCESS) {
    return base_addr;
  }
  std::cout << "[WARNING] Unable to get base address for kernel: " << zeroGetKernelName(kernel) << std::endl;
  return 0;
}

void
zeroDumpKernelProfiles
(
  const std::string& data_dir_
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
      kpfs << it->second->kernel_id_ << std::endl;
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
      zeroLogKernelProfiles(it->second, size);
#endif
    }
    kpfs.close();
  }

  kernel_command_properties_mutex_.unlock();
}
