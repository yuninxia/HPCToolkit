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
    {&EuStalls::control_, GPU_INST_STALL_OTHER},  // TBD
    {&EuStalls::pipe_, GPU_INST_STALL_PIPE_BUSY},
    {&EuStalls::send_, GPU_INST_STALL_GMEM},      // TBD
    {&EuStalls::dist_, GPU_INST_STALL_PIPE_BUSY}, // TBD
    {&EuStalls::sbid_, GPU_INST_STALL_IDEPEND},   // TBD
    {&EuStalls::sync_, GPU_INST_STALL_SYNC},
    {&EuStalls::insfetch_, GPU_INST_STALL_IFETCH},
    {&EuStalls::other_, GPU_INST_STALL_OTHER}
};

static bool
convertPCSampling
(
  const std::map<uint64_t, EuStalls>::iterator& eustall_iter,               // [in] iterator to current EU stall entry (address -> stall info)
  const std::map<uint64_t, KernelProperties>::const_iterator& kernel_iter,  // [in] iterator to kernel properties entry (base address -> properties)
  uint64_t correlation_id,                                                  // [in] unique identifier to correlate related activities
  gpu_inst_stall_t stall_reason,                                            // [in] type of stall that occurred
  uint64_t stall_count,                                                     // [in] number of times this stall occurred
  gpu_activity_t* activity                                                  // [out] activity structure to be filled with PC sampling data
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
  const std::map<uint64_t, EuStalls>::iterator& eustall_iter,
  const std::map<uint64_t, KernelProperties>::const_iterator& kernel_iter,
  uint64_t correlation_id,
  std::deque<gpu_activity_t*>& activities
)
{
  const EuStalls& stall = eustall_iter->second;

  for (const auto& mapping : stall_mappings) {
    uint64_t stall_count = stall.*(mapping.stall_value);
    if (stall_count == 0) continue; // Skip if no stalls of this type

    auto activity = std::make_unique<gpu_activity_t>();
    gpu_activity_init(activity.get());
    if (convertPCSampling(eustall_iter, kernel_iter, correlation_id, mapping.reason, stall_count, activity.get())) {
      activities.push_back(activity.release());
    }
  }
}
