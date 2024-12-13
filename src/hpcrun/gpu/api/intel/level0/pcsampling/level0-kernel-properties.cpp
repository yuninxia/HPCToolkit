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


//*****************************************************************************
// local variables
//*****************************************************************************

static std::mutex g_kprops_mutex;
static std::unordered_map<std::string, std::map<uint64_t, KernelProperties>> g_kprops_cache;
static std::unordered_map<std::string, std::filesystem::file_time_type> g_kprops_timestamps;
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
  // Create a unique cache key combining directory and device ID
  // This allows caching data for different device/directory combinations
  std::string cache_key = data_dir_name + "_" + std::to_string(device_id);
  
  // Construct the expected file path for kernel properties
  // Format: <data_dir>/.kprops.<device_id>.<pid>.txt
  std::string kprops_filename = ".kprops." + std::to_string(device_id) + "." + 
                                std::to_string(getpid()) + ".txt";
  std::filesystem::path file_path = std::filesystem::path(data_dir_name) / kprops_filename;
  
  // Check cache status under mutex protection
  // This section handles cache lookup and validation
  {
    std::lock_guard<std::mutex> lock(g_kprops_mutex);  // Thread-safe cache access
    
    // If file exists, verify if we have valid cached data
    if (std::filesystem::exists(file_path)) {
      // Get current file timestamp for cache validation
      auto current_timestamp = std::filesystem::last_write_time(file_path);
      
      // Look up cache entries
      auto cache_it = g_kprops_cache.find(cache_key);
      auto timestamp_it = g_kprops_timestamps.find(cache_key);
      
      // Check if we have a valid cache entry with matching timestamp
      // This prevents using stale cache data if file has been modified
      if (cache_it != g_kprops_cache.end() && 
          timestamp_it != g_kprops_timestamps.end() &&
          timestamp_it->second == current_timestamp) {
        // Cache hit - return cached data without reading file
        kprops = cache_it->second;
        return;
      }
    }
  }
  
  // Cache miss or file modified - need to read from file
  std::map<uint64_t, KernelProperties> new_props;
  if (std::filesystem::exists(file_path)) {
    // Open and read the kernel properties file
    std::ifstream kpf(file_path);
    if (kpf.is_open()) {
      // Read file contents line by line
      // Format: name, base_address, kernel_id, module_id, size (one per line)
      while (!kpf.eof()) {
        KernelProperties props;
        std::string line;

        // Read kernel name
        std::getline(kpf, props.name);
        if (kpf.eof()) break;

        // Read and parse base address
        std::getline(kpf, line);
        if (kpf.eof()) break;
        props.base_address = std::strtoul(line.c_str(), nullptr, 0);

        // Read kernel and module IDs
        std::getline(kpf, props.kernel_id);
        std::getline(kpf, props.module_id);

        // Read and parse size
        std::getline(kpf, line);
        props.size = std::strtoul(line.c_str(), nullptr, 0);

        // Store properties in map using base address as key
        new_props.insert({props.base_address, std::move(props)});
      }
      kpf.close();
      
      // Update cache with new data under mutex protection
      std::lock_guard<std::mutex> lock(g_kprops_mutex);
      g_kprops_cache[cache_key] = new_props;
      g_kprops_timestamps[cache_key] = std::filesystem::last_write_time(file_path);
      kprops = std::move(new_props);
    }
  } else {
    // File doesn't exist - clean up any stale cache entries
    std::lock_guard<std::mutex> lock(g_kprops_mutex);
    g_kprops_cache.erase(cache_key);
    g_kprops_timestamps.erase(cache_key);
    kprops.clear();
  }
}

void
zeroInitializeKernelBaseAddressFunction
(
  const struct hpcrun_foil_appdispatch_level0* dispatch
)
{
  ze_driver_handle_t driver;
  uint32_t count = 1;
  if (f_zeDriverGet(&count, &driver, dispatch) == ZE_RESULT_SUCCESS) {
    if (zeDriverGetExtensionFunctionAddress(driver, "zexKernelGetBaseAddress", (void **)&zexKernelGetBaseAddress) != ZE_RESULT_SUCCESS) {
      zexKernelGetBaseAddress = nullptr;
    }
  }
}

uint64_t
zeroGetKernelBaseAddress
(
  ze_kernel_handle_t kernel,
  const struct hpcrun_foil_appdispatch_level0* dispatch
)
{
  uint64_t base_addr = 0;
  if (zexKernelGetBaseAddress != nullptr && zexKernelGetBaseAddress(kernel, &base_addr) == ZE_RESULT_SUCCESS) {
    return base_addr;
  }
  std::cout << "[WARNING] Unable to get base address for kernel: " << zeroGetKernelName(kernel, dispatch) << std::endl;
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
