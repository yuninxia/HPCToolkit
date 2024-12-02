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
          zeroActivityTranslate(activities, eustall_iter, kernel_iter, cid);
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
  ze_kernel_handle_t running_kernel,
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
  generateActivities(kprops, eustalls, kernel_cids, activities, running_kernel_name);
}
