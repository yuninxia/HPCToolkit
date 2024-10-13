// SPDX-FileCopyrightText: 2002-2024 Rice University
// SPDX-FileCopyrightText: 2024 Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: BSD-3-Clause

// -*-Mode: C++;-*-

//*****************************************************************************
// local includes
//*****************************************************************************

#include "level0-logging.hpp"


//******************************************************************************
// private operations
//******************************************************************************

static void
printPCSampleInfo
(
  const gpu_activity_t* activity,
  uint64_t cid,
  const std::string& kernel_name,
  uint64_t instruction_pc_lm_ip,
  uint16_t lm_id,
  uint64_t offset
)
{
  std::cout << "PC Sample" << std::endl;
  std::cout << "PC sampling: sample(pc=0x" << std::hex << activity->details.pc_sampling.pc.lm_ip
            << ", cid=" << cid
            << ", kernel_name=" << kernel_name
            << ")" << std::endl;
  std::cout << "PC sampling: normalize 0x" << std::hex << instruction_pc_lm_ip
            << " --> [" << std::dec << lm_id << ", 0x" 
            << std::hex << offset << "]" << std::endl;
}

static const std::pair<std::string, uint64_t>&
findKernelInfo
(
  uint64_t instruction_pc_lm_ip,
  const std::map<uint64_t, std::pair<std::string, uint64_t>>& kernel_info
)
{
  static const std::pair<std::string, uint64_t> unknown_kernel = {"Unknown", 0};
  auto it = kernel_info.upper_bound(instruction_pc_lm_ip);
  if (it != kernel_info.begin()) {
    --it;
  }
  return (it != kernel_info.end()) ? it->second : unknown_kernel;
}

static void
logActivity
(
  const gpu_activity_t* activity,
  const std::map<uint64_t, std::pair<std::string, uint64_t>>& kernel_info,
  std::unordered_map<uint64_t, int>& cid_count
)
{
  uint64_t instruction_pc_lm_ip = activity->details.instruction.pc.lm_ip;
  uint64_t cid = activity->details.pc_sampling.correlation_id;
  uint16_t lm_id = activity->details.pc_sampling.pc.lm_id;
  
  const auto& [kernel_name, kernel_base] = findKernelInfo(instruction_pc_lm_ip, kernel_info);
  uint64_t offset = (kernel_base != 0) ? (instruction_pc_lm_ip - kernel_base) : 0;

  printPCSampleInfo(activity, cid, kernel_name, instruction_pc_lm_ip, lm_id, offset);
  cid_count[cid]++;
}

static void
printCorrelationIdStatistics
(
  const std::unordered_map<uint64_t, int>& cid_count
)
{
  std::cout << std::dec << std::endl;
  std::cout << "Correlation ID Statistics:" << std::endl;
  for (const auto& [cid, count] : cid_count) {
    std::cout << "Correlation ID: " << cid << " Count: " << count << std::endl;
  }
}


//******************************************************************************
// interface operations
//******************************************************************************

void 
zeroLogActivities
(
  const std::deque<gpu_activity_t*>& activities,
  const std::map<uint64_t, KernelProperties>& kprops
)
{
  // FIXME(Yuning): address adjustment is not robust
  const uint64_t BASE_ADJUSTMENT = 0x800000000000;
  std::map<uint64_t, std::pair<std::string, uint64_t>> kernel_info;
  for (const auto& [base, prop] : kprops) {
    uint64_t adjusted_base = base + BASE_ADJUSTMENT;
    kernel_info[adjusted_base] = {prop.name, adjusted_base};
  }

  std::cout << std::endl;

  std::unordered_map<uint64_t, int> cid_count;

  for (const auto* activity : activities) {
    logActivity(activity, kernel_info, cid_count);
  }

  printCorrelationIdStatistics(cid_count);
}

void
zeroLogPCSample
(
  uint64_t correlation_id,
  const KernelProperties& kernel_props,
  const gpu_pc_sampling_t& pc_sampling,
  const EuStalls& stall,
  uint64_t base_address
)
{
  std::cout << "[PC_Sample]" << std::endl;
  std::cout << "PC sampling: sample(pc=0x" << std::hex << pc_sampling.pc.lm_ip
            << ", cid=" << correlation_id
            << ", kernel_name=" << kernel_props.name
            << ")" << std::endl;
  
  uint64_t offset = pc_sampling.pc.lm_ip - base_address - 0x800000000000;
  std::cout << "PC sampling: normalize 0x" << std::hex << pc_sampling.pc.lm_ip
            << " --> [" << std::dec << pc_sampling.pc.lm_id << ", 0x" 
            << std::hex << offset << "]" << std::endl;
  
  std::cout << "Stall reason: " << pc_sampling.stallReason
            << ", Samples: " << pc_sampling.samples
            << ", Latency samples: " << pc_sampling.latencySamples << std::endl;
  
  std::cout << "Stall counts:"
            << " Active: " << stall.active_
            << ", Control: " << stall.control_
            << ", Pipe: " << stall.pipe_
            << ", Send: " << stall.send_
            << ", Dist: " << stall.dist_
            << ", SBID: " << stall.sbid_
            << ", Sync: " << stall.sync_
            << ", Insfetch: " << stall.insfetch_
            << ", Other: " << stall.other_ << std::endl;
  
  std::cout << std::endl;
}
