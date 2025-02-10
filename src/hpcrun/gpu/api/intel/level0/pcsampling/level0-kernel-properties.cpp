// SPDX-FileCopyrightText: 2002-2024 Rice University
// SPDX-FileCopyrightText: 2024 Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: BSD-3-Clause

// -*-Mode: C++;-*-

//*****************************************************************************
// local includes
//*****************************************************************************

#include "level0-kernel-properties.hpp"


//*****************************************************************************
// global variables
//*****************************************************************************

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

static void
level0LogKernelProfiles
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

static std::string
buildKernelPropertiesCacheKey
(
  const std::string& data_dir_name,
  int32_t device_id
)
{
  return data_dir_name + "_" + std::to_string(device_id);
}

static std::filesystem::path
buildKernelPropertiesFilePath
(
  const std::string& data_dir_name,
  int32_t device_id
)
{
  std::string filename = ".kprops." + std::to_string(device_id) + "." + std::to_string(getpid()) + ".txt";
  return std::filesystem::path(data_dir_name) / filename;
}

static bool
tryGetCachedKernelProperties
(
  const std::string& cache_key,
  const std::filesystem::path& file_path,
  std::map<uint64_t, KernelProperties>& outProps
)
{
  std::lock_guard<std::mutex> lock(g_kprops_mutex);
  if (std::filesystem::exists(file_path)) {
    auto current_timestamp = std::filesystem::last_write_time(file_path);
    auto cache_it = g_kprops_cache.find(cache_key);
    auto ts_it = g_kprops_timestamps.find(cache_key);
    if (cache_it != g_kprops_cache.end() &&
        ts_it != g_kprops_timestamps.end() &&
        ts_it->second == current_timestamp) {
      outProps = cache_it->second;
      return true;
    }
  }
  return false;
}

static std::map<uint64_t, KernelProperties>
readKernelPropertiesFromFile
(
  const std::filesystem::path& file_path
)
{
  std::map<uint64_t, KernelProperties> properties;
  std::ifstream kpf(file_path);
  if (kpf.is_open()) {
    std::string name, baseAddrStr, kernelId, moduleId, sizeStr;
    // Read file contents in groups of 5 lines.
    while (std::getline(kpf, name) &&
           std::getline(kpf, baseAddrStr) &&
           std::getline(kpf, kernelId) &&
           std::getline(kpf, moduleId) &&
           std::getline(kpf, sizeStr)) {
      KernelProperties props;
      props.name = name;
      props.base_address = std::strtoul(baseAddrStr.c_str(), nullptr, 0);
      props.kernel_id = kernelId;
      props.module_id = moduleId;
      props.size = std::strtoul(sizeStr.c_str(), nullptr, 0);
      properties.insert({ props.base_address, std::move(props) });
    }
  }
  return properties;
}

static void
updateKernelPropertiesCache
(
  const std::string& cache_key,
  const std::filesystem::path& file_path,
  const std::map<uint64_t, KernelProperties>& new_props
)
{
  std::lock_guard<std::mutex> lock(g_kprops_mutex);
  g_kprops_cache[cache_key] = new_props;
  g_kprops_timestamps[cache_key] = std::filesystem::last_write_time(file_path);
}

static void
clearKernelPropertiesCache
(
  const std::string& cache_key,
  std::map<uint64_t, KernelProperties>& kprops
)
{
  std::lock_guard<std::mutex> lock(g_kprops_mutex);
  g_kprops_cache.erase(cache_key);
  g_kprops_timestamps.erase(cache_key);
  kprops.clear();
}

static std::map<int32_t, std::map<uint64_t, ZeKernelCommandProperties*>>
buildDeviceKernelPropsMap
(
  void
)
{
  std::map<int32_t, std::map<uint64_t, ZeKernelCommandProperties*>> device_kprops;
  for (auto& entry : *kernel_command_properties_) {
    int32_t device_id = entry.second.device_id_;
    uint64_t base_addr = entry.second.base_addr_;
    device_kprops[device_id].insert({ base_addr, &entry.second });
  }
  return device_kprops;
}

static void
writeKernelProfilesToFile
(
  const std::string& data_dir,
  int32_t device_id,
  const std::map<uint64_t, ZeKernelCommandProperties*>& kprops
)
{
  std::string fpath = data_dir + "/.kprops." + std::to_string(device_id) + "." + std::to_string(getpid()) + ".txt";
  std::ofstream kpfs(fpath, std::ios::out | std::ios::trunc);
  if (!kpfs.is_open()) {
    // Failed to open file; optionally log an error
    return;
  }
  uint64_t prev_base = 0;
  // Iterate in reverse order to compute the kernel size from the base address
  for (auto it = kprops.crbegin(); it != kprops.crend(); ++it) {
    kpfs << "\"" << it->second->name_ << "\"" << std::endl;
    kpfs << it->second->base_addr_ << std::endl;
    kpfs << it->second->kernel_id_ << std::endl;
    kpfs << it->second->module_id_ << std::endl;

    size_t size = 0;
    if (prev_base == 0) {
      size = it->second->size_;
    } else {
      size = prev_base - it->second->base_addr_;
      if (size > it->second->size_) {
        size = it->second->size_;
      }
    }
    kpfs << size << std::endl;
    prev_base = it->second->base_addr_;

#if 0
    level0LogKernelProfiles(it->second, size);
#endif
  }
  kpfs.close();
}

//******************************************************************************
// interface operations
//******************************************************************************

void
level0InitializeKernelCommandProperties
(
  void
)
{
  std::lock_guard<std::shared_mutex> lock(kernel_command_properties_mutex_);
  if (kernel_command_properties_ == nullptr) {
    kernel_command_properties_ = new std::map<std::string, ZeKernelCommandProperties>;
  }
}

void
level0ReadKernelProperties
(
  const int32_t device_id,
  const std::string& data_dir_name,
  std::map<uint64_t, KernelProperties>& kprops
)
{
  // Create a unique cache key combining directory and device ID
  std::string cache_key = buildKernelPropertiesCacheKey(data_dir_name, device_id);
  // Construct the expected file path for the kernel properties file
  std::filesystem::path file_path = buildKernelPropertiesFilePath(data_dir_name, device_id);

  // Attempt to retrieve cached data (under mutex protection)
  if (tryGetCachedKernelProperties(cache_key, file_path, kprops)) {
    // Cache hit: return the cached kernel properties
    return;
  }

  // Cache miss or file modification: read from file
  if (std::filesystem::exists(file_path)) {
    auto new_props = readKernelPropertiesFromFile(file_path);
    updateKernelPropertiesCache(cache_key, file_path, new_props);
    kprops = std::move(new_props);
  } else {
    // File does not exist: clear any stale cache entries
    clearKernelPropertiesCache(cache_key, kprops);
  }
}

void
level0InitializeKernelBaseAddressFunction
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
level0GetKernelBaseAddress
(
  ze_kernel_handle_t kernel,
  const struct hpcrun_foil_appdispatch_level0* dispatch
)
{
  uint64_t base_addr = 0;
  if (zexKernelGetBaseAddress != nullptr && zexKernelGetBaseAddress(kernel, &base_addr) == ZE_RESULT_SUCCESS) {
    return base_addr;
  }
  std::cout << "[WARNING] Unable to get base address for kernel: " << level0GetKernelName(kernel, dispatch) << std::endl;
  return 0;
}

void
level0DumpKernelProfiles
(
  const std::string& data_dir_
)
{
  // Dump kernel profiles into files grouped by device ID
  std::lock_guard<std::shared_mutex> lock(kernel_command_properties_mutex_);
  if (!kernel_command_properties_) {
    // If the kernel command properties map is not initialized, do nothing
    return;
  }
  auto device_kprops = buildDeviceKernelPropsMap();
  for (const auto& device_entry : device_kprops) {
    int32_t device_id = device_entry.first;
    writeKernelProfilesToFile(data_dir_, device_id, device_entry.second);
  }
}
