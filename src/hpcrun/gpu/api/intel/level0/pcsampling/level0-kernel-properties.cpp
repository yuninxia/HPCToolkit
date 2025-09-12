// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: BSD-3-Clause

// -*-Mode: C++;-*-

//*****************************************************************************
// system includes
//*****************************************************************************

#include <algorithm>


//*****************************************************************************
// local includes
//*****************************************************************************

#include "level0-kernel-properties.hpp"
#include "level0-kernel-properties-cache.hpp"


//*****************************************************************************
// global variables
//*****************************************************************************

std::shared_mutex kernel_command_properties_mutex_;
std::map<std::string, ZeKernelCommandProperties> *kernel_command_properties_ = nullptr;


//*****************************************************************************
// local variables
//*****************************************************************************

static ze_result_t (*zexKernelGetBaseAddress)(ze_kernel_handle_t hKernel, uint64_t *baseAddress) = nullptr;



//******************************************************************************
// private operations
//******************************************************************************


// Store kernel properties in memory cache
static void
storeKernelPropertiesInCache
(
  const ZeKernelCommandProperties& props
)
{
  KernelPropertiesCache::getInstance().storeKernelProperties(props.kernel_id_, props);
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
  std::map<uint64_t, KernelProperties>& kprops
)
{
  // Use high-performance memory cache
  // Try RCU cache first for lock-free reading
  auto rcu_data = KernelPropertiesRCU::getInstance().read(device_id);
  if (rcu_data) {
    kprops = rcu_data->properties;
    if (std::getenv("HPCTOOLKIT_LEVEL0_DEBUG_CACHE")) {
      std::cout << "[DEBUG] Read " << kprops.size() << " kernel properties from RCU cache for device " << device_id << std::endl;
    }
    return;
  }
  
  // Fallback to regular cache if RCU not available
  if (std::getenv("HPCTOOLKIT_LEVEL0_DEBUG_CACHE")) {
    std::cout << "[DEBUG] RCU cache miss for device " << device_id << ", falling back to regular cache" << std::endl;
  }
  KernelPropertiesCache::getInstance().getKernelProperties(device_id, kprops);
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
  void
)
{
  // Store all pending kernel properties to memory cache
  std::lock_guard<std::shared_mutex> lock(kernel_command_properties_mutex_);
  if (kernel_command_properties_) {
    for (const auto& [kernel_id, props] : *kernel_command_properties_) {
      storeKernelPropertiesInCache(props);
    }
  }
  
  // Print cache statistics in debug mode
  if (std::getenv("HPCTOOLKIT_LEVEL0_DEBUG_CACHE")) {
    auto stats = KernelPropertiesCache::getInstance().getStats();
    std::cout << "[INFO] Kernel cache stats: "
              << stats.total_entries << " entries, "
              << stats.devices << " devices, "
              << "avg read: " << stats.avg_read_time.count() << "us, "
              << "avg write: " << stats.avg_write_time.count() << "us, "
              << "memory: " << stats.memory_bytes / 1024 << "KB" << std::endl;
    
    // Optionally print detailed cache contents
    // KernelPropertiesCache::getInstance().debugPrint();
  }
}
