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

static void
convertStallReason
(
  const EuStalls& stall,
  gpu_inst_stall_t& stall_reason
) 
{
  stall_reason = GPU_INST_STALL_NONE;
  uint64_t max_value = 0;

  // FIXME(Yuning): stall reasons are not accurately mapped
  // Map stall types to reasons
  const struct {
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

  // Iterate through the mappings to find the maximum stall value
  for (const auto& mapping : stall_mappings) {
    if (stall.*(mapping.stall_value) > max_value) {
      max_value = stall.*(mapping.stall_value);
      stall_reason = mapping.reason;
    }
  }
}

static bool
convertPCSampling
(
  gpu_activity_t* activity, 
  const std::map<uint64_t, EuStalls>::iterator& it,
  const std::map<uint64_t, KernelProperties>::const_reverse_iterator& rit,
  uint64_t correlation_id
)
{
  if (!activity) return false;

  activity->kind = GPU_ACTIVITY_PC_SAMPLING;

  const KernelProperties& kernel_props = rit->second;
  const EuStalls& stall = it->second;

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
  std::cout << "real: " << std::hex << it->first << " ,base: " << std::hex << rit->first << " ,offset: " << std::hex << it->first - rit->first << std::endl;
#endif

  // FIXME(Yuning): address adjustment is not robust
  activity->details.pc_sampling.pc.lm_ip = it->first + 0x800000000000; // real = it->first; base = rit->first; offset = real - base;
  activity->details.pc_sampling.correlation_id = correlation_id;
  activity->details.pc_sampling.samples = stall.active_ + stall.control_ + stall.pipe_ + 
    stall.send_ + stall.dist_ + stall.sbid_ + stall.sync_ + stall.insfetch_ + stall.other_;
  activity->details.pc_sampling.latencySamples = activity->details.pc_sampling.samples - stall.active_;  
  convertStallReason(stall, activity->details.pc_sampling.stallReason);
  
  return true;
}


//******************************************************************************
// interface operations
//******************************************************************************

void
zeroActivityTranslate
(
  std::deque<gpu_activity_t*>& activities, 
  const std::map<uint64_t, EuStalls>::iterator& it,
  const std::map<uint64_t, KernelProperties>::const_reverse_iterator& rit,
  uint64_t correlation_id
) 
{
  auto activity = std::make_unique<gpu_activity_t>();
  gpu_activity_init(activity.get());

  if (convertPCSampling(activity.get(), it, rit, correlation_id)) {
    activities.push_back(activity.release());
  }
}
