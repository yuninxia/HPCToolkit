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
static std::string
toHex
(
  T value
)
{
  std::stringstream ss;
  ss << "0x" << std::hex << std::setw(sizeof(T) * 2) << std::setfill('0') << value;
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
            << ", cid=" << cid
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

static std::map<uint64_t, std::pair<std::string, uint64_t>>
buildKernelInfoMap
(
  const std::map<uint64_t, KernelProperties>& kprops
)
{
  std::map<uint64_t, std::pair<std::string, uint64_t>> kernel_info;
  for (const auto& [base, prop] : kprops) {
    kernel_info[base] = {prop.name, base};
  }
  return kernel_info;
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

static std::string
formatMetricValue
(
  const zet_typed_value_t& metric
)
{
  std::stringstream ss;
  switch (metric.type) {
    case ZET_VALUE_TYPE_UINT32:
      ss << "UINT32, Value = " << metric.value.ui32;
      break;
    case ZET_VALUE_TYPE_UINT64:
      ss << "UINT64, Value = " << metric.value.ui64;
      break;
    case ZET_VALUE_TYPE_FLOAT32:
      ss << "FLOAT32, Value = " << metric.value.fp32;
      break;
    case ZET_VALUE_TYPE_FLOAT64:
      ss << "FLOAT64, Value = " << metric.value.fp64;
      break;
    case ZET_VALUE_TYPE_BOOL8:
      ss << "BOOL8, Value = " << (metric.value.b8 ? "true" : "false");
      break;
    default:
      ss << "Unknown type";
  }
  return ss.str();
}

static void
logMetricsForSample
(
  size_t sampleIndex,
  uint32_t metricCount,
  const std::vector<zet_typed_value_t>& metrics
)
{
  std::cout << "Sample " << sampleIndex << ": " << metricCount << " metrics\n";
  // Compute the starting index for this sample
  for (uint32_t j = 0; j < metricCount; ++j) {
    size_t metricIndex = sampleIndex * metricCount + j;
    if (metricIndex < metrics.size()) {
      std::cout << "  Metric " << j << ": Type = "
                << formatMetricValue(metrics[metricIndex]) << "\n";
    }
  }
}

//******************************************************************************
// interface operations
//******************************************************************************

void
level0LogActivities
(
  const std::deque<gpu_activity_t*>& activities,
  const std::map<uint64_t, KernelProperties>& kprops
)
{
  // Build a map from kernel base addresses to kernel names
  auto kernel_info = buildKernelInfoMap(kprops);

  std::cout << '\n';
  std::unordered_map<uint64_t, int> cid_count;

  // Log each GPU activity
  for (const auto* activity : activities) {
    logActivity(activity, kernel_info, cid_count);
  }

  // Print correlation ID statistics
  printCorrelationIdStatistics(cid_count);
}

void
level0LogPCSample
(
  uint64_t correlation_id,
  const KernelProperties& kernel_props,
  const gpu_pc_sampling_t& pc_sampling,
  const EuStalls& stall,
  uint64_t base_address
)
{
  // Calculate the offset of the PC sampling address from the kernel base
  uint64_t offset = pc_sampling.pc.lm_ip - base_address;
  
  std::cout << "[PC_Sample]\n";
  logPCSampleInfo(pc_sampling.pc.lm_ip, correlation_id, kernel_props.name, pc_sampling.pc.lm_id, offset);
  std::cout << "Stall reason: " << pc_sampling.stallReason
            << ", Samples: " << pc_sampling.samples
            << ", Latency samples: " << pc_sampling.latencySamples << "\n"
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
level0LogMetricList
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
level0LogSamplesAndMetrics
(
  const std::vector<uint32_t>& samples,
  const std::vector<zet_typed_value_t>& metrics
)
{
  std::cout << "\nSamples and Metrics\n";
  std::cout << "samples: " << samples.size() << '\n';
  std::cout << "metrics: " << metrics.size() << '\n';

  // Iterate over each sample and log its metrics.
  for (size_t i = 0; i < samples.size(); ++i) {
    logMetricsForSample(i, samples[i], metrics);
  }
  std::cout << '\n';
}
