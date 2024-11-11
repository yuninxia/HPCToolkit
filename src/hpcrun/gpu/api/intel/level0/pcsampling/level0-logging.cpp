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

// Helper function to format hexadecimal output
template<typename T>
std::string
toHex
(
  T value
)
{
  std::stringstream ss;
  ss << "0x" << std::hex << value;
  return ss.str();
}

static void
logPCSampleInfo
(
  uint64_t pc,
  uint64_t cid,
  const std::string& kernel_name,
  uint16_t lm_id,
  uint64_t offset
)
{
  std::cout << "PC sampling: sample(pc=" << toHex(pc)
            << ", cid=" << toHex(cid)
            << ", kernel_name=" << kernel_name << ")\n"
            << "PC sampling: normalize " << toHex(pc)
            << " --> [" << lm_id << ", " << toHex(offset) << "]\n";
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

  std::cout << "PC Sample\n";
  logPCSampleInfo(activity->details.pc_sampling.pc.lm_ip, cid, kernel_name, lm_id, offset);
  ++cid_count[cid];
}

static void
printCorrelationIdStatistics
(
  const std::unordered_map<uint64_t, int>& cid_count
)
{
  std::cout << "\nCorrelation ID Statistics:\n";
  for (const auto& [cid, count] : cid_count) {
    std::cout << "Correlation ID: " << cid << " Count: " << count << '\n';
  }
  std::cout << '\n';
}

void
extractThreadIdAndSeqNum
(
  uint64_t cid,
  uint64_t& thread_id,
  uint64_t& seq_num
)
{
  thread_id = cid >> 32;          // High 32 bits -> thread ID
  seq_num = cid & 0xFFFFFFFF;     // Low 32 bits -> sequence number
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
  std::map<uint64_t, std::pair<std::string, uint64_t>> kernel_info;
  for (const auto& [base, prop] : kprops) {
    kernel_info[base] = {prop.name, base};
  }

  std::cout << '\n';
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
  uint64_t offset = pc_sampling.pc.lm_ip - base_address;
  
  std::cout << "[PC_Sample]\n";
  logPCSampleInfo(pc_sampling.pc.lm_ip, correlation_id, kernel_props.name, pc_sampling.pc.lm_id, offset);
  std::cout << "Stall reason: " << pc_sampling.stallReason
            << ", Samples: " << pc_sampling.samples
            << ", Latency samples: " << pc_sampling.latencySamples << '\n'
            << "Stall counts: Active: " << stall.active_
            << ", Control: " << stall.control_
            << ", Pipe: " << stall.pipe_
            << ", Send: " << stall.send_
            << ", Dist: " << stall.dist_
            << ", SBID: " << stall.sbid_
            << ", Sync: " << stall.sync_
            << ", Insfetch: " << stall.insfetch_
            << ", Other: " << stall.other_ << "\n\n";
}

void
zeroLogMetricList
(
  const std::vector<std::string>& metric_list
)
{
  std::cout << "\nMetric list:\n";
  std::cout << "metric_list.size(): " << metric_list.size() << '\n';
  for (const auto& metric : metric_list) {
    std::cout << "metric_list: " << metric << '\n';
  }
  std::cout << '\n';
}

void
zeroLogSamplesAndMetrics
(
  const std::vector<uint32_t>& samples,
  const std::vector<zet_typed_value_t>& metrics
)
{
  std::cout << "\nSamples and Metrics\n";
  std::cout << "samples: " << samples.size() << '\n';
  std::cout << "metrics: " << metrics.size() << '\n';

  for (size_t i = 0; i < samples.size(); ++i) {
    std::cout << "Sample " << i << ": " << samples[i] << " metrics\n";
    for (uint32_t j = 0; j < samples[i]; ++j) {
      const auto& metric = metrics[i * samples[i] + j];
      std::cout << "  Metric " << j << ": Type = ";
      switch (metric.type) {
        case ZET_VALUE_TYPE_UINT32:
          std::cout << "UINT32, Value = " << metric.value.ui32;
          break;
        case ZET_VALUE_TYPE_UINT64:
          std::cout << "UINT64, Value = " << metric.value.ui64;
          break;
        case ZET_VALUE_TYPE_FLOAT32:
          std::cout << "FLOAT32, Value = " << metric.value.fp32;
          break;
        case ZET_VALUE_TYPE_FLOAT64:
          std::cout << "FLOAT64, Value = " << metric.value.fp64;
          break;
        case ZET_VALUE_TYPE_BOOL8:
          std::cout << "BOOL8, Value = " << (metric.value.b8 ? "true" : "false");
          break;
        default:
          std::cout << "Unknown type";
      }
      std::cout << '\n';
    }
  }
  std::cout << '\n';
}

void
zeroLogTimingData
(
  const KernelTimingData& timing_data
)
{
  std::cout << "\nKernel Execution Time and Launch Count Distribution:" << std::endl;
  std::cout << std::string(110, '=') << std::endl;
  
  // Process each kernel (base PC) separately
  for (const auto& [base_pc, info] : timing_data.pc_timing_map) {
    // Calculate total duration for this kernel and total launch count
    uint64_t kernel_total_duration = 0;
    uint64_t kernel_total_launch_count = 0;
    for (const auto& timing_info : info) {
      kernel_total_duration += timing_info.duration_ns;
      kernel_total_launch_count += timing_info.kernel_launch_count;
    }
    
    // Print kernel header
    std::cout << "Kernel@0x" << std::hex << base_pc << std::dec << std::endl;
    std::cout << std::string(110, '-') << std::endl;
    
    std::cout << std::left << std::setw(15) << "CID"
              << std::left << std::setw(15) << "Thread ID"
              << std::left << std::setw(15) << "Seq Number"
              << std::left << std::setw(15) << "Duration (ns)"
              << std::left << std::setw(15) << "Duration (%)"
              << std::left << std::setw(15) << "Launch Count"
              << std::left << std::setw(15) << "Launch Ratio (%)"
              << std::endl;
    
    std::cout << std::string(110, '-') << std::endl;
    
    for (const auto& timing_info : info) {
      uint64_t thread_id = 0;
      uint64_t seq_num = 0;
      extractThreadIdAndSeqNum(timing_info.cid, thread_id, seq_num);
      
      double duration_percentage = (static_cast<double>(timing_info.duration_ns) / kernel_total_duration) * 100.0;
      
      double launch_ratio = (kernel_total_launch_count > 0) ?
                            (static_cast<double>(timing_info.kernel_launch_count) / kernel_total_launch_count) * 100.0 :
                            0.0;
      
      std::cout << std::left << std::setw(15) << (toHex(timing_info.cid))
                << std::left << std::setw(15) << (toHex(thread_id))
                << std::left << std::setw(15) << (toHex(seq_num))
                << std::left << std::setw(15) << timing_info.duration_ns
                << std::left << std::setw(15) << std::fixed << std::setprecision(2) << duration_percentage
                << std::left << std::setw(15) << timing_info.kernel_launch_count
                << std::left << std::setw(15) << std::fixed << std::setprecision(2) << launch_ratio
                << std::endl;
    }
    
    std::cout << std::string(110, '-') << std::endl;
    std::cout << "Total Duration: " << kernel_total_duration << " ns" << std::endl;
    std::cout << "Total Launch Count: " << kernel_total_launch_count << "\n" << std::endl;
  }
}
