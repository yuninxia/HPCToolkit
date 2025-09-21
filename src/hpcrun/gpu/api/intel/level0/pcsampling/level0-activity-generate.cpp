// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: BSD-3-Clause

// -*-Mode: C++;-*-

//*****************************************************************************
// system includes
//*****************************************************************************

#include <algorithm>
#include <deque>


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
  const std::string& stripped_kernel_name  // Already stripped kernel name
)
{
  auto it = kernel_cids.find(stripped_kernel_name);
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
  // Since we can't reliably match GPU virtual addresses with kernel handle addresses,
  // associate all EU stalls with the currently running kernel.
  // This is valid because we're collecting samples while a specific kernel is executing.

  if (kernel_ranges.empty() || kprops.empty()) {
    return;
  }

  // Get the first kernel's properties (should be the only one for the running kernel)
  const auto& first_range = kernel_ranges[0];
  auto kernel_iter = kprops.find(first_range.first);
  if (kernel_iter == kprops.end()) {
    return;
  }

  // Associate all EU stalls with this kernel
  for (auto eustall_iter = eustalls.begin(); eustall_iter != eustalls.end(); ++eustall_iter) {
    // The EU stall address is a GPU virtual address - pass it through as-is
    // The analysis tools will handle address translation appropriately
    level0ActivityTranslate(eustall_iter, kernel_iter, cid, activities);
  }
}

static std::vector<std::pair<uint64_t, uint64_t>>
collectKernelRanges
(
  const std::map<uint64_t, KernelProperties>& kprops,
  const std::string& stripped_kernel_name  // Already stripped kernel name
)
{
  std::vector<std::pair<uint64_t, uint64_t>> ranges;
  // Iterate in forward order to preserve ascending order of addresses.
  for (const auto& entry : kprops) {
    std::string prop_name_stripped = stripEdgeQuotes(entry.second.name);
    if (prop_name_stripped == stripped_kernel_name) {
      ranges.emplace_back(entry.first, entry.first + entry.second.size);
    }
  }
  return ranges;
}

static std::unordered_map<std::string, uint64_t>
generateKernelCorrelationIds
(
  const std::map<uint64_t, KernelProperties>& kprops,  // [in] map from kernel base address to kernel properties
  uint64_t correlation_id                              // [in] unique identifier for correlating kernel activities
)
{
  std::unordered_map<std::string, uint64_t> kernel_cids;
  // Build a map of unique kernel names to the current correlation ID
  // Note: All instances of the same kernel name will share the same correlation ID
  for (const auto& [addr, props] : kprops) {
    std::string stripped_name = stripEdgeQuotes(props.name);
    // Only insert if this kernel name hasn't been seen before
    kernel_cids.try_emplace(stripped_name, correlation_id);
  }
  return kernel_cids;
}

static void
generateActivities
(
  const std::map<uint64_t, KernelProperties>& kprops,            // [in] map of kernel addresses to their properties
  std::map<uint64_t, EuStalls>& eustalls,                        // [in] map of EU stall addresses to stall information
  const std::unordered_map<std::string, uint64_t>& kernel_cids,  // [in] map of kernel names to correlation IDs
  const std::string& stripped_kernel_name,                       // [in] stripped name of currently running kernel
  std::deque<gpu_activity_t*>& activities                        // [out] queue for generated activities
)
{
  // Return early if no kernel is running
  if (stripped_kernel_name.empty()) return;

  // Collect kernel address ranges matching the running kernel.
  auto kernel_ranges = collectKernelRanges(kprops, stripped_kernel_name);
  if (kernel_ranges.empty()) return; // No matching kernel instances found.

  // Retrieve the correlation ID for the running kernel.
  uint64_t cid = getCorrelationId(kernel_cids, stripped_kernel_name);

  // Process EU stall events and generate activities.
  processEuStalls(kprops, eustalls, kernel_ranges, cid, activities);
}


//******************************************************************************
// interface operations
//******************************************************************************

void
level0GenerateActivities
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

  // Activities should be empty when called
  activities.clear();

  // Extract the running kernel name and strip quotes once
  std::string running_kernel_name = level0GetKernelName(running_kernel, dispatch);
  std::string stripped_kernel_name = stripEdgeQuotes(running_kernel_name);

  // Generate kernel correlation IDs using stripped kernel names
  auto kernel_cids = generateKernelCorrelationIds(kprops, correlation_id);

  // Generate GPU activities based on kernel properties and EU stall events
  generateActivities(kprops, eustalls, kernel_cids, stripped_kernel_name, activities);
}
