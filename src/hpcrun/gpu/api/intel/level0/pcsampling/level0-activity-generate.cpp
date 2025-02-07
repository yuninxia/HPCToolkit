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

static uint64_t
getCorrelationId
(
  const std::unordered_map<std::string, uint64_t>& kernel_cids,
  const std::string& running_kernel_name
)
{
  std::string stripped_name = stripEdgeQuotes(running_kernel_name);
  auto it = kernel_cids.find(stripped_name);
  if (it != kernel_cids.end()) {
    return it->second;
  }
  return 0;
}

static void
processEuStalls
(
  const std::map<uint64_t, KernelProperties>& kprops,
  std::map<uint64_t, EuStalls>& eustalls,
  const std::vector<std::pair<uint64_t, uint64_t>>& kernel_ranges,
  uint64_t cid,
  std::deque<gpu_activity_t*>& activities
)
{
  // Process each EU stall event.
  for (auto eustall_iter = eustalls.begin(); eustall_iter != eustalls.end(); ++eustall_iter) {
    uint64_t stall_addr = eustall_iter->first;

    // Use binary search to locate a kernel range that might include the stall address.
    auto it = std::lower_bound(kernel_ranges.begin(), kernel_ranges.end(),
      std::make_pair(stall_addr, UINT64_MAX),
      [](const std::pair<uint64_t, uint64_t>& range, const std::pair<uint64_t, uint64_t>& value) {
        return range.first < value.first;
      });

    // Check the previous range in case lower_bound overshoots.
    if (it != kernel_ranges.begin()) {
      --it;
      if (stall_addr >= it->first && stall_addr < it->second) {
        // Retrieve kernel properties using the start address.
        auto kernel_iter = kprops.find(it->first);
        if (kernel_iter != kprops.end()) {
          zeroActivityTranslate(eustall_iter, kernel_iter, cid, activities);
        }
      }
    }
  }
}

static std::vector<std::pair<uint64_t, uint64_t>>
collectKernelRanges
(
  const std::map<uint64_t, KernelProperties>& kprops,
  const std::string& running_kernel_name
)
{
  std::vector<std::pair<uint64_t, uint64_t>> ranges;
  std::string stripped_name = stripEdgeQuotes(running_kernel_name);
  // Iterate in forward order to preserve ascending order of addresses.
  for (const auto& entry : kprops) {
    if (stripEdgeQuotes(entry.second.name) == stripped_name) {
      ranges.emplace_back(entry.first, entry.first + entry.second.size);
    }
  }
  return ranges;
}

static std::unordered_map<std::string, uint64_t>
generateKernelCorrelationIds
(
  const std::map<uint64_t, KernelProperties>& kprops,  // [in] map from kernel base address to kernel properties
  uint64_t& correlation_id                             // [in] unique identifier for correlating kernel activities
)
{
  std::unordered_map<std::string, uint64_t> kernel_cids;
  // Iterate in forward order (ascending addresses) for consistency
  for (auto it = kprops.begin(); it != kprops.end(); ++it) {
    // Use the stripped kernel name as the key
    kernel_cids.emplace(stripEdgeQuotes(it->second.name), correlation_id);
  }
  return kernel_cids;
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
  // Return early if no kernel is running
  if (running_kernel_name.empty()) return;

  // Collect kernel address ranges matching the running kernel.
  auto kernel_ranges = collectKernelRanges(kprops, running_kernel_name);
  if (kernel_ranges.empty()) return; // No matching kernel instances found.

  // Retrieve the correlation ID for the running kernel.
  uint64_t cid = getCorrelationId(kernel_cids, running_kernel_name);

  // Process EU stall events and generate activities.
  processEuStalls(kprops, eustalls, kernel_ranges, cid, activities);
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

  // Clear any existing activities
  activities.clear();

  // Extract the running kernel name
  std::string running_kernel_name = zeroGetKernelName(running_kernel, dispatch);

  // Generate kernel correlation IDs using stripped kernel names
  auto kernel_cids = generateKernelCorrelationIds(kprops, correlation_id);

  // Generate GPU activities based on kernel properties and EU stall events
  generateActivities(kprops, eustalls, kernel_cids, running_kernel_name, activities);
}
