// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: Apache-2.0

// -*-Mode: C++;-*-

#ifndef LEVEL0_KERNEL_PROPERTIES_CACHE_H
#define LEVEL0_KERNEL_PROPERTIES_CACHE_H

//*****************************************************************************
// system includes
//*****************************************************************************

#include <atomic>
#include <chrono>
#include <map>
#include <memory>
#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

//*****************************************************************************
// local includes
//*****************************************************************************

#include "level0-kernel-properties.hpp"

//*****************************************************************************
// class definition
//*****************************************************************************

/**
 * Thread-safe in-memory cache for kernel properties.
 * Replaces file-based storage with high-performance shared memory.
 */
class KernelPropertiesCache {
public:
  // Singleton pattern for global cache instance
  static KernelPropertiesCache& getInstance() {
    static KernelPropertiesCache instance;
    return instance;
  }

  /**
   * Store kernel command properties in cache.
   * Thread-safe with write lock.
   */
  void storeKernelProperties(const std::string& kernel_id,
                            const ZeKernelCommandProperties& props);

  /**
   * Retrieve kernel properties for a specific device.
   * Thread-safe with read lock.
   */
  void getKernelProperties(int32_t device_id,
                          std::map<uint64_t, KernelProperties>& out_props);

  /**
   * Get all kernel properties grouped by device.
   * Used for final data export if needed.
   */
  std::map<int32_t, std::vector<const ZeKernelCommandProperties*>>
  getAllPropertiesByDevice() const;

  /**
   * Clear all cached data.
   */
  void clear();

  /**
   * Get cache statistics for debugging.
   */
  struct CacheStats {
    size_t total_entries;
    size_t devices;
    std::chrono::microseconds avg_read_time;
    std::chrono::microseconds avg_write_time;
    size_t memory_bytes;
  };
  CacheStats getStats() const;

  /**
   * Print cache contents for debugging.
   */
  void debugPrint() const;


private:
  KernelPropertiesCache() = default;
  ~KernelPropertiesCache() = default;

  // Prevent copying
  KernelPropertiesCache(const KernelPropertiesCache&) = delete;
  KernelPropertiesCache& operator=(const KernelPropertiesCache&) = delete;

  // Convert internal properties to output format
  KernelProperties convertToKernelProperties(const ZeKernelCommandProperties& cmd_props) const;

  // Compute kernel size based on adjacent kernels
  size_t computeKernelSize(const ZeKernelCommandProperties& props,
                          const std::map<uint64_t, ZeKernelCommandProperties>& device_kernels) const;

private:
  // Main cache storage: kernel_id -> properties
  mutable std::shared_mutex cache_mutex_;
  std::unordered_map<std::string, ZeKernelCommandProperties> kernel_cache_;

  // Device mapping: device_id -> set of kernel_ids
  mutable std::shared_mutex device_mutex_;
  std::unordered_map<int32_t, std::unordered_set<std::string>> device_kernels_;

  // Performance monitoring
  mutable std::atomic<uint64_t> read_count_{0};
  mutable std::atomic<uint64_t> write_count_{0};
  mutable std::atomic<uint64_t> total_read_ns_{0};
  mutable std::atomic<uint64_t> total_write_ns_{0};
};

//*****************************************************************************
// Performance optimization: Lock-free reader pattern
//*****************************************************************************

/**
 * Lock-free kernel properties reader using RCU pattern.
 * For ultra-high-frequency reads in the sampling loop.
 */
class KernelPropertiesRCU {
public:
  struct KernelData {
    std::map<uint64_t, KernelProperties> properties;
    std::atomic<uint64_t> version{0};
  };

  static KernelPropertiesRCU& getInstance() {
    static KernelPropertiesRCU instance;
    return instance;
  }

  /**
   * Update kernel data (writer side - infrequent).
   */
  void update(int32_t device_id, std::map<uint64_t, KernelProperties>&& new_data);

  /**
   * Read kernel data (reader side - high frequency).
   * Returns shared_ptr for safe concurrent access.
   */
  std::shared_ptr<const KernelData> read(int32_t device_id) const;

private:
  KernelPropertiesRCU() = default;

  // RCU storage: device_id -> atomic pointer to immutable data
  mutable std::shared_mutex update_mutex_;
  std::unordered_map<int32_t, std::shared_ptr<KernelData>> rcu_data_;
};

#endif // LEVEL0_KERNEL_PROPERTIES_CACHE_H
