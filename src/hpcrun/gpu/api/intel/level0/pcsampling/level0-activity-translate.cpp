// SPDX-FileCopyrightText: 2002-2024 Rice University
// SPDX-FileCopyrightText: 2024 Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: BSD-3-Clause

// -*-Mode: C++;-*-

//*****************************************************************************
// local includes
//*****************************************************************************

#include "level0-activity-translate.hpp"


//******************************************************************************
// private operations
//******************************************************************************

// FIXME(Yuning): stall reasons are not accurately mapped
// Map stall types to reasons
static const struct {
  uint64_t EuStalls::* stall_value;
  gpu_inst_stall_t reason;
} stall_mappings[] = {
    {&EuStalls::control_, GPU_INST_STALL_OTHER}, // TBD
    {&EuStalls::pipe_, GPU_INST_STALL_PIPE_BUSY},
    {&EuStalls::send_, GPU_INST_STALL_GMEM}, // TBD
    {&EuStalls::dist_, GPU_INST_STALL_PIPE_BUSY}, // TBD
    {&EuStalls::sbid_, GPU_INST_STALL_IDEPEND}, // TBD
    {&EuStalls::sync_, GPU_INST_STALL_SYNC},
    {&EuStalls::insfetch_, GPU_INST_STALL_IFETCH},
    {&EuStalls::other_, GPU_INST_STALL_OTHER}
};

static bool
convertPCSampling
(
  gpu_activity_t* activity, 
  const std::map<uint64_t, EuStalls>::iterator& eustall_iter,
  const std::map<uint64_t, KernelProperties>::const_reverse_iterator& kernel_iter,
  uint64_t correlation_id,
  gpu_inst_stall_t stall_reason,
  uint64_t stall_count
)
{
  if (!activity) return false;

  activity->kind = GPU_ACTIVITY_PC_SAMPLING;

  const KernelProperties& kernel_props = kernel_iter->second;

  // Convert hex string to uint32_t directly
  uint32_t module_id_uint32 = 0;
  std::istringstream iss(kernel_props.module_id.substr(0, 8));
  iss >> std::hex >> module_id_uint32;

  zebin_id_map_entry_t* entry = zebin_id_map_lookup(module_id_uint32);
  if (entry) {
    uint32_t hpctoolkit_module_id = zebin_id_map_entry_hpctoolkit_id_get(entry);
    activity->details.pc_sampling.pc.lm_id = static_cast<uint16_t>(hpctoolkit_module_id);
  }

#if 0
  uint64_t real = eustall_iter->first;
  uint64_t base = kernel_iter->first;
  uint64_t offset = real - base;
  std::cout << "[INFO] real: " << std::hex << real << " ,base: " << std::hex << base << " ,offset: " << std::hex << offset << std::endl;
#endif

  activity->details.pc_sampling.pc.lm_ip = eustall_iter->first;
  activity->details.pc_sampling.correlation_id = correlation_id;
  activity->details.pc_sampling.samples = stall_count;

  // FIXME(Yuning): latencySamples may not be accurate
  activity->details.pc_sampling.latencySamples = stall_count;
  activity->details.pc_sampling.stallReason = stall_reason;

#if 0
  zeroLogPCSample(correlation_id, kernel_props, activity->details.pc_sampling, eustall_iter->second, kernel_iter->first);
#endif

  return true;
}


//******************************************************************************
// interface operations
//******************************************************************************

void
zeroActivityTranslate
(
  std::deque<gpu_activity_t*>& activities, 
  const std::map<uint64_t, EuStalls>::iterator& eustall_iter,
  const std::map<uint64_t, KernelProperties>::const_reverse_iterator& kernel_iter,
  uint64_t correlation_id,
  double ratio = 1.0
)
{
  const EuStalls& stalls = eustall_iter->second;

  for (const auto& mapping : stall_mappings) {
    // Calculate stall count (apply ratio if not 1.0)
    uint64_t original_count = stalls.*(mapping.stall_value);
    uint64_t stall_count = (ratio == 1.0) ? original_count : static_cast<uint64_t>(std::round(original_count * ratio));
    
    if (stall_count == 0) continue; // Skip if no stalls of this type

    auto activity = std::make_unique<gpu_activity_t>();
    if (!activity) {
      std::cerr << "Failed to allocate memory for activity" << std::endl;
      continue;
    }

    gpu_activity_init(activity.get());
    if (convertPCSampling(activity.get(), eustall_iter, kernel_iter, correlation_id, mapping.reason, stall_count)) {
      activities.push_back(activity.release());
    }
  }
}

void
zeroActivityTranslateWithRatio
(
  uint64_t pc,
  const EuStalls& stalls,
  const std::map<uint64_t, KernelProperties>::const_reverse_iterator& kernel_iter,
  uint64_t correlation_id,
  double ratio,
  std::deque<gpu_activity_t*>& activities
)
{
  // Create temporary map to hold stall data for this PC
  std::map<uint64_t, EuStalls> temp_map;
  temp_map[pc] = stalls;
  
  // Use the common translate function with ratio
  zeroActivityTranslate(activities, temp_map.begin(), kernel_iter, correlation_id, ratio);
}
