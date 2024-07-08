#ifndef LEVEL0_METRIC_H
#define LEVEL0_METRIC_H

#include <cstdint>
#include <string>
#include <map>
#include <vector>
#include <level_zero/zet_api.h>

namespace l0_metric {

struct EuStalls {
  uint64_t active_;
  uint64_t control_;
  uint64_t pipe_;
  uint64_t send_;
  uint64_t dist_;
  uint64_t sbid_;
  uint64_t sync_;
  uint64_t insfetch_;
  uint64_t other_;
};

std::string 
GetMetricUnits
(
  const char* units
);

uint32_t 
GetMetricId
(
  const std::vector<std::string>& metric_list, 
  const std::string& metric_name
);

uint32_t 
GetMetricCount
(
  zet_metric_group_handle_t group
);

std::vector<std::string> 
GetMetricList
(
  zet_metric_group_handle_t group
);

zet_metric_group_handle_t 
GetMetricGroup
(
  ze_device_handle_t device, 
  const std::string& metric_group_name
);

uint64_t
CollectMetrics
(
  zet_metric_streamer_handle_t streamer, 
  std::vector<uint8_t>& storage
);

std::map<uint64_t, EuStalls>
CalculateEuStalls
(
  zet_metric_group_handle_t metric_group,
  int raw_size,
  const std::vector<uint8_t>& raw_metrics
);

}  // namespace l0_metric

#endif  // LEVEL0_METRIC_H