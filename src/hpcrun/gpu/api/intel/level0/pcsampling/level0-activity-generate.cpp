// SPDX-FileCopyrightText: 2002-2024 Rice University
// SPDX-FileCopyrightText: 2024 Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: BSD-3-Clause

// -*-Mode: C++;-*-

//*****************************************************************************
// local includes
//*****************************************************************************

#include "level0-activity-generate.hpp"


//******************************************************************************
// private operations
//******************************************************************************

static std::unordered_map<std::string, uint64_t>
generateKernelCorrelationIds
(
  const std::map<uint64_t, KernelProperties>& kprops,  // [in] map from kernel base address to kernel properties
  uint64_t& correlation_id                             // [in] unique identifier for correlating kernel activities
)
{
  std::unordered_map<std::string, uint64_t> kernel_cids;
  for (auto rit = kprops.crbegin(); rit != kprops.crend(); ++rit) {
    kernel_cids.emplace(rit->second.name, correlation_id);
  }
  return kernel_cids;
}

static std::string
stripEdgeQuotes
(
  const std::string& str
)
{
  if (str.length() < 2) return str;
  
  if (str.front() == '"' && str.back() == '"') {
    return str.substr(1, str.length() - 2);
  }
  return str;
}

static void
generateActivities
(
  const std::map<uint64_t, KernelProperties>& kprops,            // [in] map of kernel addresses to their properties
  std::map<uint64_t, EuStalls>& eustalls,                        // [in] map of EU stall addresses to stall information
  const std::unordered_map<std::string, uint64_t>& kernel_cids,  // [in] map of kernel names to correlation IDs
  const std::string& running_kernel_name,                        // [in] name of currently running kernel
  std::deque<gpu_activity_t*>& activities                        // [out] queue for generated activities
)
{
  // Early return if no kernel is running
  // Prevents unnecessary processing when there's no active kernel
  if (running_kernel_name.empty()) return;

  // Pre-collect all address ranges for the running kernel
  // Each pair contains (start_address, end_address) for a kernel instance
  std::vector<std::pair<uint64_t, uint64_t>> kernel_ranges;
  for (auto it = kprops.crbegin(); it != kprops.crend(); ++it) {
    if (stripEdgeQuotes(it->second.name) == running_kernel_name) {
      kernel_ranges.emplace_back(it->first, it->first + it->second.size);
    }
  }

  // Early return if no matching kernel instances found
  // Prevents unnecessary processing when kernel name doesn't match any known kernels
  if (kernel_ranges.empty()) return;

  // Cache the correlation ID for the running kernel
  uint64_t cid = 0;
  for (const auto& kernel : kprops) {
    if (stripEdgeQuotes(kernel.second.name) == running_kernel_name) {
      cid = kernel_cids.at(kernel.second.name);
      break;
    }
  }

  // Process each EU stall event
  for (auto eustall_iter = eustalls.begin(); eustall_iter != eustalls.end(); ++eustall_iter) {
    uint64_t eustall_addr = eustall_iter->first;
    
    // Use binary search to efficiently find potential matching kernel range
    // This is more efficient than linear search through kernel properties
    auto it = std::lower_bound(kernel_ranges.begin(), kernel_ranges.end(),
      std::make_pair(eustall_addr, UINT64_MAX),
      [](const auto& range, const auto& value) {
        return range.first < value.first;
      });

    // Step back to check previous range since lower_bound returns the first range whose start
    // is >= eustall_addr, but the stall might belong to a previous range. For example:
    // If ranges are [1000,2000), [3000,4000) and stall_addr=1500, lower_bound returns
    // [3000,4000) but the stall actually belongs to [1000,2000)
    if (it != kernel_ranges.begin()) {
      --it;  // Move to previous range
      if (eustall_addr >= it->first && eustall_addr < it->second) {
        // Found matching kernel range, retrieve kernel properties and generate activity
        auto kernel_iter = kprops.find(it->first);
        if (kernel_iter != kprops.end()) {
            zeroActivityTranslate(eustall_iter, kernel_iter, cid, activities);
        }
      }
    }
  }
}


//******************************************************************************
// interface operations
//******************************************************************************

void
zeroGenerateActivities
(
  const std::map<uint64_t, KernelProperties>& kprops, 
  std::map<uint64_t, EuStalls>& eustalls,
  uint64_t& correlation_id,
  ze_kernel_handle_t running_kernel,
  std::deque<gpu_activity_t*>& activities,
  const struct hpcrun_foil_appdispatch_level0* dispatch
)
{
  if (running_kernel == nullptr) {
    return;
  }

  activities.clear();

  // Extract kernel name
  std::string running_kernel_name = zeroGetKernelName(running_kernel, dispatch);

  // Generate kernel correlation IDs
  auto kernel_cids = generateKernelCorrelationIds(kprops, correlation_id);

  // Generate activities
  generateActivities(kprops, eustalls, kernel_cids, running_kernel_name, activities);
}
