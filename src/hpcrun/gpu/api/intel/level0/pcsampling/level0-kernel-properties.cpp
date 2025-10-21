// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: BSD-3-Clause

// -*-Mode: C++;-*-

//*****************************************************************************
// system includes
//*****************************************************************************

#include <algorithm>
#include <cstdlib>


//*****************************************************************************
// local includes
//*****************************************************************************

#include "level0-kernel-properties.hpp"
#include "level0-kernel-properties-cache.hpp"
#include "level0-pc-api-receiver.hpp"
#include "level0-driver.hpp"


//*****************************************************************************
// global variables
//*****************************************************************************


//*****************************************************************************
// local variables
//*****************************************************************************

static ze_result_t (*zexKernelGetBaseAddress)(ze_kernel_handle_t hKernel, uint64_t *baseAddress) = nullptr;



//******************************************************************************
// private operations
//******************************************************************************




//******************************************************************************
// interface operations
//******************************************************************************


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
      pcsampling::warn("[DEBUG] Read %zu kernel properties from RCU cache for device %d",
                       kprops.size(), device_id);
    }
    return;
  }

  // Fallback to regular cache if RCU not available
  if (std::getenv("HPCTOOLKIT_LEVEL0_DEBUG_CACHE")) {
    pcsampling::warn("[DEBUG] RCU cache miss for device %d, falling back to regular cache", device_id);
  }
  KernelPropertiesCache::getInstance().getKernelProperties(device_id, kprops);
}

void
level0InitializeKernelBaseAddressFunction
(
  const struct hpcrun_foil_appdispatch_level0* dispatch
)
{
  ze_driver_handle_t driver = nullptr;
  uint32_t count = 1;
  ze_result_t status = pcsampling::callZeDriverGet(&count, &driver, dispatch);
  if (status != ZE_RESULT_SUCCESS || driver == nullptr) {
    zexKernelGetBaseAddress = nullptr;
    return;
  }

  void* addr = nullptr;
  status = pcsampling::callZeDriverGetExtensionFunctionAddress(
      driver, "zexKernelGetBaseAddress", &addr, dispatch);

  if (status == ZE_RESULT_SUCCESS && addr != nullptr) {
    zexKernelGetBaseAddress = reinterpret_cast<decltype(zexKernelGetBaseAddress)>(addr);
  } else {
    zexKernelGetBaseAddress = nullptr;
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
  std::string kernel_name = level0GetKernelName(kernel, dispatch);
  pcsampling::warn("Unable to get base address for kernel: %s", kernel_name.c_str());
  return 0;
}

void
level0DumpKernelProfiles
(
  void
)
{
  // Print cache statistics in debug mode
  if (std::getenv("HPCTOOLKIT_LEVEL0_DEBUG_CACHE")) {
    auto stats = KernelPropertiesCache::getInstance().getStats();
    pcsampling::warn("[INFO] Kernel cache stats: %zu entries, %zu devices, avg read: %lld us, avg write: %lld us, memory: %zu KB",
                     stats.total_entries, stats.devices,
                     static_cast<long long>(stats.avg_read_time.count()),
                     static_cast<long long>(stats.avg_write_time.count()),
                     stats.memory_bytes / 1024);

    // Optionally print detailed cache contents
    // KernelPropertiesCache::getInstance().debugPrint();
  }
}
