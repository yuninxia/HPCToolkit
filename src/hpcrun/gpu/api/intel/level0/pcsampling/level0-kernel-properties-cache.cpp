// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: BSD-3-Clause

// -*-Mode: C++;-*-

//*****************************************************************************
// system includes
//*****************************************************************************

#include <algorithm>
#include <iostream>
#include <mutex>

//*****************************************************************************
// local includes
//*****************************************************************************

#include "level0-kernel-properties-cache.hpp"

//*****************************************************************************
// KernelPropertiesCache implementation
//*****************************************************************************

void
KernelPropertiesCache::storeKernelProperties(
  const std::string& kernel_id,
  const ZeKernelCommandProperties& props)
{
  auto start_time = std::chrono::high_resolution_clock::now();

  {
    std::unique_lock<std::shared_mutex> lock(cache_mutex_);
    kernel_cache_[kernel_id] = props;
  }

  {
    std::unique_lock<std::shared_mutex> lock(device_mutex_);
    device_kernels_[props.device_id_].insert(kernel_id);
  }

  auto end_time = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time);

  write_count_.fetch_add(1, std::memory_order_relaxed);
  total_write_ns_.fetch_add(duration.count(), std::memory_order_relaxed);

  // Update RCU cache for lock-free reading
  std::map<uint64_t, KernelProperties> device_props;
  getKernelProperties(props.device_id_, device_props);
  if (std::getenv("HPCTOOLKIT_LEVEL0_DEBUG_CACHE")) {
    std::cout << "[DEBUG] Updating RCU cache with " << device_props.size()
              << " kernels for device " << props.device_id_ << std::endl;
  }
  KernelPropertiesRCU::getInstance().update(props.device_id_, std::move(device_props));
}

void
KernelPropertiesCache::getKernelProperties(
  int32_t device_id,
  std::map<uint64_t, KernelProperties>& out_props)
{
  auto start_time = std::chrono::high_resolution_clock::now();
  out_props.clear();

  // Get list of kernel IDs for this device
  std::unordered_set<std::string> kernel_ids;
  {
    std::shared_lock lock(device_mutex_);
    auto it = device_kernels_.find(device_id);
    if (it != device_kernels_.end()) {
      kernel_ids = it->second;
    }
  }

  // Build map of all kernels for this device
  std::map<uint64_t, ZeKernelCommandProperties> device_kernel_map;
  {
    std::shared_lock lock(cache_mutex_);
    for (const auto& kid : kernel_ids) {
      auto it = kernel_cache_.find(kid);
      if (it != kernel_cache_.end() && it->second.device_id_ == device_id) {
        device_kernel_map[it->second.base_addr_] = it->second;
      }
    }
  }

  // Convert to output format with computed sizes
  for (const auto& [base_addr, cmd_props] : device_kernel_map) {
    KernelProperties props = convertToKernelProperties(cmd_props);
    props.size = computeKernelSize(cmd_props, device_kernel_map);
    out_props[base_addr] = props;
  }

  if (std::getenv("HPCTOOLKIT_LEVEL0_DEBUG_CACHE")) {
    std::cout << "[DEBUG] getKernelProperties: returning " << out_props.size()
              << " kernels for device " << device_id << std::endl;
  }

  auto end_time = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time);

  read_count_.fetch_add(1, std::memory_order_relaxed);
  total_read_ns_.fetch_add(duration.count(), std::memory_order_relaxed);
}

KernelProperties
KernelPropertiesCache::convertToKernelProperties(
  const ZeKernelCommandProperties& cmd_props) const
{
  KernelProperties props;
  props.name = cmd_props.name_;
  props.base_address = cmd_props.base_addr_;
  props.kernel_id = cmd_props.kernel_id_;
  props.module_id = cmd_props.module_id_;
  props.size = cmd_props.size_;
  props.sample_count = 0;
  return props;
}

size_t
KernelPropertiesCache::computeKernelSize(
  const ZeKernelCommandProperties& props,
  const std::map<uint64_t, ZeKernelCommandProperties>& device_kernels) const
{
  // Find the next kernel with higher base address
  auto it = device_kernels.find(props.base_addr_);
  if (it != device_kernels.end()) {
    ++it;
    if (it != device_kernels.end()) {
      // Size is difference between this kernel and next kernel
      size_t computed_size = it->first - props.base_addr_;
      // Use the minimum of computed size and declared size
      return std::min(computed_size, static_cast<size_t>(props.size_));
    }
  }
  // Last kernel or only kernel - use declared size
  return props.size_;
}

std::map<int32_t, std::vector<const ZeKernelCommandProperties*>>
KernelPropertiesCache::getAllPropertiesByDevice() const
{
  std::shared_lock cache_lock(cache_mutex_);
  std::shared_lock device_lock(device_mutex_);

  std::map<int32_t, std::vector<const ZeKernelCommandProperties*>> result;

  for (const auto& [device_id, kernel_ids] : device_kernels_) {
    auto& device_props = result[device_id];
    for (const auto& kid : kernel_ids) {
      auto it = kernel_cache_.find(kid);
      if (it != kernel_cache_.end()) {
        device_props.push_back(&it->second);
      }
    }
    // Sort by base address for consistent output
    std::sort(device_props.begin(), device_props.end(),
              [](const auto* a, const auto* b) {
                return a->base_addr_ < b->base_addr_;
              });
  }

  return result;
}

void
KernelPropertiesCache::clear()
{
  std::unique_lock<std::shared_mutex> cache_lock(cache_mutex_);
  std::unique_lock<std::shared_mutex> device_lock(device_mutex_);

  kernel_cache_.clear();
  device_kernels_.clear();

  read_count_ = 0;
  write_count_ = 0;
  total_read_ns_ = 0;
  total_write_ns_ = 0;
}

KernelPropertiesCache::CacheStats
KernelPropertiesCache::getStats() const
{
  std::shared_lock cache_lock(cache_mutex_);
  std::shared_lock device_lock(device_mutex_);

  CacheStats stats;
  stats.total_entries = kernel_cache_.size();
  stats.devices = device_kernels_.size();

  uint64_t reads = read_count_.load(std::memory_order_relaxed);
  uint64_t writes = write_count_.load(std::memory_order_relaxed);
  uint64_t read_ns = total_read_ns_.load(std::memory_order_relaxed);
  uint64_t write_ns = total_write_ns_.load(std::memory_order_relaxed);

  stats.avg_read_time = reads > 0 ?
    std::chrono::microseconds(read_ns / reads / 1000) :
    std::chrono::microseconds(0);
  stats.avg_write_time = writes > 0 ?
    std::chrono::microseconds(write_ns / writes / 1000) :
    std::chrono::microseconds(0);

  // Estimate memory usage
  stats.memory_bytes = stats.total_entries * sizeof(ZeKernelCommandProperties) +
                      stats.total_entries * 64 + // Estimated string overhead per kernel
                      stats.devices * sizeof(std::unordered_set<std::string>) +
                      stats.total_entries * 64; // Estimated overhead for kernel ID strings

  return stats;
}

void
KernelPropertiesCache::debugPrint() const
{
  if (!std::getenv("HPCTOOLKIT_LEVEL0_DEBUG_CACHE")) {
    return;  // Only print in debug mode
  }

  std::shared_lock<std::shared_mutex> cache_lock(cache_mutex_);
  std::shared_lock<std::shared_mutex> device_lock(device_mutex_);

  std::cout << "\n[DEBUG] Kernel Properties Cache Contents:\n";
  std::cout << "========================================\n";

  for (const auto& [device_id, kernels] : device_kernels_) {
    std::cout << "Device " << device_id << ": " << kernels.size() << " kernels\n";

    for (const auto& kernel_id : kernels) {
      auto it = kernel_cache_.find(kernel_id);
      if (it != kernel_cache_.end()) {
        const auto& props = it->second;
        std::cout << "  Kernel: " << props.name_ << "\n"
                  << "    ID: " << props.kernel_id_ << "\n"
                  << "    Base: 0x" << std::hex << props.base_addr_ << std::dec << "\n"
                  << "    Size: " << props.size_ << " bytes\n";
      }
    }
  }

  std::cout << "========================================\n" << std::endl;
}


//*****************************************************************************
// KernelPropertiesRCU implementation
//*****************************************************************************

void
KernelPropertiesRCU::update(
  int32_t device_id,
  std::map<uint64_t, KernelProperties>&& new_data)
{
  auto new_kernel_data = std::make_shared<KernelData>();
  new_kernel_data->properties = std::move(new_data);

  std::unique_lock<std::shared_mutex> lock(update_mutex_);

  // Get current version and increment
  uint64_t new_version = 1;
  auto it = rcu_data_.find(device_id);
  if (it != rcu_data_.end() && it->second) {
    new_version = it->second->version.load() + 1;
  }
  new_kernel_data->version = new_version;

  // Update the map - shared_ptr assignment is atomic
  rcu_data_[device_id] = new_kernel_data;
}

std::shared_ptr<const KernelPropertiesRCU::KernelData>
KernelPropertiesRCU::read(int32_t device_id) const
{
  // Use shared_lock for concurrent reads - multiple readers allowed
  // This is still efficient as writers are rare (only on kernel creation)
  std::shared_lock lock(update_mutex_);

  auto it = rcu_data_.find(device_id);
  if (it != rcu_data_.end()) {
    return it->second;
  }

  return nullptr;
}
