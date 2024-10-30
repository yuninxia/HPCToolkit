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
  const std::map<uint64_t, KernelProperties>& kprops,
  uint64_t& correlation_id
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
  const std::map<uint64_t, KernelProperties>& kprops,
  std::map<uint64_t, EuStalls>& eustalls,
  const std::unordered_map<std::string, uint64_t>& kernel_cids,
  std::deque<gpu_activity_t*>& activities,
  const std::string& running_kernel_name
)
{
  for (auto eustall_iter = eustalls.begin(); eustall_iter != eustalls.end(); ++eustall_iter) {
    for (auto kernel_iter = kprops.crbegin(); kernel_iter != kprops.crend(); ++kernel_iter) {
      uint64_t instruction_offset = eustall_iter->first - kernel_iter->first;
      if ((instruction_offset >= 0) && (instruction_offset < kernel_iter->second.size)) {
        std::string stripped_name = stripEdgeQuotes(kernel_iter->second.name);
        if (stripped_name == running_kernel_name) {
          uint64_t cid = kernel_cids.at(kernel_iter->second.name);
          zeroActivityTranslate(activities, eustall_iter, kernel_iter, cid, 1.0);
        }
        break;
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
  std::deque<gpu_activity_t*>& activities,
  ze_kernel_handle_t running_kernel
)
{
  if (running_kernel == nullptr) {
    return;
  }

  activities.clear();

  // Extract kernel name
  std::string running_kernel_name = zeroGetKernelName(running_kernel);

  // Generate kernel correlation IDs
  auto kernel_cids = generateKernelCorrelationIds(kprops, correlation_id);

  // Generate activities
  generateActivities(kprops, eustalls, kernel_cids, activities, running_kernel_name);
}

void
zeroGenerateActivitiesFromAccumulatedMetrics
(
  const std::map<uint64_t, KernelProperties>& kprops,
  const std::map<uint64_t, EuStalls>& accumulated_eustalls,
  const KernelTimingData& timing_data,
  std::deque<gpu_activity_t*>& activities
)
{
#if 1
  zeroLogTimingData(timing_data);
#endif

  activities.clear();
  
  // For each real PC in accumulated metrics
  for (const auto& [real_pc, stalls] : accumulated_eustalls) {
    // Find which kernel this real PC belongs to
    auto kernel_iter = kprops.crbegin();
    for (; kernel_iter != kprops.crend(); ++kernel_iter) {
      uint64_t base_pc = kernel_iter->first;
      uint64_t size = kernel_iter->second.size;
      
      // Check if real_pc falls within this kernel's range
      if (real_pc >= base_pc && real_pc < base_pc + size) {
        auto timing_it = timing_data.pc_timing_map.find(base_pc);
        if (timing_it == timing_data.pc_timing_map.end() || timing_it->second.empty()) continue;

        // Calculate total duration for this kernel
        uint64_t kernel_total_duration = 0;
        // uint64_t total_launch_count = 0;
        for (const auto& info : timing_it->second) {
          kernel_total_duration += info.duration_ns;
          // total_launch_count += info.kernel_launch_count;
        }

        // Distribute metrics based on timing ratios
        for (const auto& info : timing_it->second) {
          // Calculate the ratio of time spent in this context
          double ratio = static_cast<double>(info.duration_ns) / kernel_total_duration;
          // double ratio = static_cast<double>(info.kernel_launch_count) / total_launch_count;

          // Generate activities with proportionally distributed stall counts
          zeroActivityTranslateWithRatio(real_pc, stalls, kernel_iter, info.cid, ratio, activities);
        }
        break;  // Found the kernel, no need to continue searching
      }
    }
  }
}
