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

std::string
getKernelName
(
  ze_kernel_handle_t kernel
)
{
  size_t name_len = 0;
  ze_result_t status = zeKernelGetName(kernel, &name_len, nullptr);
  if (status != ZE_RESULT_SUCCESS || name_len == 0) {
    return "UnknownKernel";
  }

  std::vector<char> kernel_name(name_len);
  status = zeKernelGetName(kernel, &name_len, kernel_name.data());
  return (status == ZE_RESULT_SUCCESS) ? std::string(kernel_name.begin(), kernel_name.end() - 1) : "UnknownKernel";
}

std::unordered_map<std::string, uint64_t>
generateKernelCorrelationIds
(
  const std::map<uint64_t, KernelProperties>& kprops,
  uint64_t& correlation_id
)
{
  std::unordered_map<std::string, uint64_t> kernel_cids;
  for (auto rit = kprops.crbegin(); rit != kprops.crend(); ++rit) {
    if (kernel_cids.find(rit->second.name) == kernel_cids.end()) {
      kernel_cids[rit->second.name] = correlation_id;
    }
  }
  return kernel_cids;
}

std::string
stripEdgeQuotes
(
  const std::string& str
)
{
  if (str.length() < 2) {
      return str;
  }
  size_t start = 0, end = str.length() - 1;
  if (str[start] == '"' && str[end] == '"') {
      return str.substr(start + 1, end - start - 1);
  }
  return str;
}

void
generateActivities
(
  const std::map<uint64_t, KernelProperties>& kprops,
  std::map<uint64_t, EuStalls>& eustalls,
  const std::unordered_map<std::string, uint64_t>& kernel_cids,
  std::deque<gpu_activity_t*>& activities,
  const std::string& running_kernel_name
)
{
  for (auto it = eustalls.begin(); it != eustalls.end(); ++it) {
    uint64_t stall_time = it->first;
    for (auto rit = kprops.crbegin(); rit != kprops.crend(); ++rit) {
      if ((rit->first <= stall_time) && ((stall_time - rit->first) < rit->second.size)) {
        std::string stripped_name = stripEdgeQuotes(rit->second.name);
        if (stripped_name == running_kernel_name) {
          uint64_t cid = kernel_cids.at(rit->second.name);
          zeroActivityTranslate(activities, it, rit, cid);
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
  std::string running_kernel_name = getKernelName(running_kernel);

  // Generate kernel correlation IDs
  auto kernel_cids = generateKernelCorrelationIds(kprops, correlation_id);

  // Generate activities
  generateActivities(kprops, eustalls, kernel_cids, activities, running_kernel_name);
}
