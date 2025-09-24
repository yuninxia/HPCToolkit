// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: BSD-3-Clause

// -*-Mode: C++;-*-

//*****************************************************************************
// system includes
//*****************************************************************************

#include <cstdlib>
#include <iostream>
#include <memory>


//*****************************************************************************
// local includes
//*****************************************************************************

#include "level0-activity-translate.hpp"
#include "level0-pc-api-receiver.hpp"


//******************************************************************************
// private operations
//******************************************************************************

// FIXME(Yuning): stall reasons are not accurately mapped
// Map stall types to reasons
static const struct {
  uint64_t EuStalls::* stall_value;
  gpu_inst_stall_t reason;
} stall_mappings[] = {
    {&EuStalls::control_,   GPU_INST_STALL_NOT_SELECTED}, // Control flow stalls
    {&EuStalls::pipe_,      GPU_INST_STALL_PIPE_BUSY},    // Pipeline stalls
    {&EuStalls::send_,      GPU_INST_STALL_GMEM},         // Send operation waits on memory
    {&EuStalls::dist_,      GPU_INST_STALL_MEM_THROTTLE}, // Dispatch throttling
    {&EuStalls::sbid_,      GPU_INST_STALL_IDEPEND},      // Scoreboard dependency
    {&EuStalls::sync_,      GPU_INST_STALL_SYNC},         // Synchronization stalls
    {&EuStalls::insfetch_,  GPU_INST_STALL_IFETCH},       // Instruction fetch stalls
    {&EuStalls::other_,     GPU_INST_STALL_OTHER}         // Other stalls
};

static void
setPCSamplingModuleId
(
  gpu_activity_t* activity,
  const KernelProperties& kernel_props
)
{
  // Convert the first 8 characters of the hex string to a uint32_t
  uint32_t module_id_uint32 = std::strtoul(kernel_props.module_id.substr(0, 8).c_str(), nullptr, 16);

  zebin_id_map_entry_t* entry = pcsampling::lookupZebinIdMap(module_id_uint32);
  if (entry) {
    uint32_t hpctoolkit_module_id = pcsampling::getZebinIdMapEntryHpctoolkitId(entry);
    activity->details.pc_sampling.pc.lm_id = static_cast<uint16_t>(hpctoolkit_module_id);
  }
}

static void
fillPCSamplingActivityFields
(
  gpu_activity_t* activity,
  uint64_t lm_ip,
  uint64_t correlation_id,
  uint64_t stall_count,
  gpu_inst_stall_t stall_reason
)
{
  activity->details.pc_sampling.pc.lm_ip = lm_ip;
  activity->details.pc_sampling.correlation_id = correlation_id;
  activity->details.pc_sampling.samples = stall_count;

  // FIXME(Yuning): latencySamples may not be accurate
  activity->details.pc_sampling.latencySamples = stall_count;
  activity->details.pc_sampling.stallReason = stall_reason;
}

static bool
convertPCSampling
(
  const std::map<uint64_t, EuStalls>::iterator& eustall_iter,
  const std::map<uint64_t, KernelProperties>::const_iterator& kernel_iter,
  uint64_t correlation_id,
  gpu_inst_stall_t stall_reason,
  uint64_t stall_count,
  gpu_activity_t* activity
)
{
  if (!activity) return false;

  activity->kind = GPU_ACTIVITY_PC_SAMPLING;
  const KernelProperties& kernel_props = kernel_iter->second;

  // Set the module id (lm_id) using the kernel properties
  setPCSamplingModuleId(activity, kernel_props);

  // Debug: Print address offset information if debugging is enabled
  if (std::getenv("HPCTOOLKIT_LEVEL0_DEBUG_PCSAMPLE")) {
    uint64_t real = eustall_iter->first;
    uint64_t base = kernel_iter->first;
    uint64_t offset = real - base;
    std::cerr << "[DEBUG] PC Sample - real: 0x" << std::hex << real
              << ", base: 0x" << base
              << ", offset: 0x" << offset << std::dec << std::endl;
  }

  // Fill in the remaining fields
  fillPCSamplingActivityFields(activity, eustall_iter->first, correlation_id, stall_count, stall_reason);

  // Debug: Log detailed PC sample information if debugging is enabled
  if (std::getenv("HPCTOOLKIT_LEVEL0_DEBUG_PCSAMPLE")) {
    level0LogPCSample(correlation_id, kernel_props, activity->details.pc_sampling, eustall_iter->second, kernel_iter->first);
  }

  return true;
}

static gpu_activity_t*
createAndFillActivity
(
  const std::map<uint64_t, EuStalls>::iterator& eustall_iter,
  const std::map<uint64_t, KernelProperties>::const_iterator& kernel_iter,
  uint64_t correlation_id,
  gpu_inst_stall_t stall_reason,
  uint64_t stall_count
)
{
  gpu_activity_t* activity = pcsampling::allocActivity();
  if (!activity) return nullptr;

  pcsampling::initActivity(activity);

  if (convertPCSampling(eustall_iter, kernel_iter, correlation_id, stall_reason, stall_count, activity)) {
    return activity;
  } else {
    pcsampling::freeActivity(activity);
    return nullptr;
  }
}

//******************************************************************************
// interface operations
//******************************************************************************

void
level0ActivityTranslate
(
  const std::map<uint64_t, EuStalls>::iterator& eustall_iter,
  const std::map<uint64_t, KernelProperties>::const_iterator& kernel_iter,
  uint64_t correlation_id,
  std::deque<gpu_activity_t*>& activities
)
{
  const EuStalls& stall = eustall_iter->second;

  // Iterate over all stall mappings
  for (const auto& mapping : stall_mappings) {
    // Get the count of stalls for the current mapping
    uint64_t stall_count = stall.*(mapping.stall_value);
    if (stall_count == 0) continue; // Skip if no stalls of this type

    // Create and fill a new GPU activity
    gpu_activity_t* activity = createAndFillActivity(eustall_iter, kernel_iter, correlation_id, mapping.reason, stall_count);
    if (activity) activities.push_back(activity);
  }
}
