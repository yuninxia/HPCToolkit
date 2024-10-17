// SPDX-FileCopyrightText: 2002-2024 Rice University
// SPDX-FileCopyrightText: 2024 Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: BSD-3-Clause

// -*-Mode: C++;-*-

//*****************************************************************************
// local includes
//*****************************************************************************

#include "level0-metric.hpp"


//******************************************************************************
// private operations
//******************************************************************************

static uint32_t
getNumberOfMetricGroups
(
  ze_device_handle_t device
)
{
  uint32_t num_groups = 0;
  ze_result_t status = zetMetricGroupGet(device, &num_groups, nullptr);
  level0_check_result(status, __LINE__);
  return num_groups;
}

static std::vector<zet_metric_group_handle_t>
getMetricGroups
(
  ze_device_handle_t device,
  uint32_t num_groups
)
{
  std::vector<zet_metric_group_handle_t> groups(num_groups, nullptr);
  ze_result_t status = zetMetricGroupGet(device, &num_groups, groups.data());
  level0_check_result(status, __LINE__);
  return groups;
}

static zet_metric_group_properties_t
getMetricGroupProperties
(
  zet_metric_group_handle_t group
)
{
  zet_metric_group_properties_t group_props{};
  group_props.stype = ZET_STRUCTURE_TYPE_METRIC_GROUP_PROPERTIES;
  ze_result_t status = zetMetricGroupGetProperties(group, &group_props);
  level0_check_result(status, __LINE__);
  return group_props;
}

static bool
isMatchingMetricGroup
(
  const zet_metric_group_properties_t& group_props,
  const std::string& metric_group_name
)
{
  return (group_props.name == metric_group_name) &&
          (group_props.samplingType & ZET_METRIC_GROUP_SAMPLING_TYPE_FLAG_TIME_BASED);
}

static zet_metric_group_handle_t
findMatchingMetricGroup
(
  const std::vector<zet_metric_group_handle_t>& groups,
  const std::string& metric_group_name
)
{
  for (auto& current_group : groups) {
    zet_metric_group_properties_t group_props = getMetricGroupProperties(current_group);
    if (isMatchingMetricGroup(group_props, metric_group_name)) {
      return current_group;
    }
  }
  return nullptr;
}

static EuStalls
createEuStalls
(
  const zet_typed_value_t* value
)
{
  return {
    .active_ = value[1].value.ui64,
    .control_ = value[2].value.ui64,
    .pipe_ = value[3].value.ui64,
    .send_ = value[4].value.ui64,
    .dist_ = value[5].value.ui64,
    .sbid_ = value[6].value.ui64,
    .sync_ = value[7].value.ui64,
    .insfetch_ = value[8].value.ui64,
    .other_ = value[9].value.ui64
  };
}

static void
updateExistingEuStalls
(
  EuStalls& existing,
  const EuStalls& stall
)
{
  existing.active_ += stall.active_;
  existing.control_ += stall.control_;
  existing.pipe_ += stall.pipe_;
  existing.send_ += stall.send_;
  existing.dist_ += stall.dist_;
  existing.sbid_ += stall.sbid_;
  existing.sync_ += stall.sync_;
  existing.insfetch_ += stall.insfetch_;
  existing.other_ += stall.other_;
}

static void
processMetricSample
(
  const zet_typed_value_t* value,
  std::map<uint64_t, EuStalls>& eustalls
)
{
  uint64_t low_ip = (value[0].value.ui64 << 3);
  if (low_ip == 0) return;
  uint64_t full_ip = 0x800000000000ULL | (low_ip & 0xFFFFFFFF);
  if (full_ip == 0) return;

  EuStalls stall = createEuStalls(value);
  auto [it, inserted] = eustalls.try_emplace(full_ip, stall);
  if (!inserted) {
    updateExistingEuStalls(it->second, stall);
  }
}


//******************************************************************************
// interface operations
//******************************************************************************

void
zeroGetMetricGroup
(
  ze_device_handle_t device,
  const std::string& metric_group_name,
  zet_metric_group_handle_t& group
)
{
  uint32_t num_groups = getNumberOfMetricGroups(device);
  if (num_groups == 0) {
    std::cerr << "[WARNING] No metric groups found" << std::endl;
    group = nullptr;
    return;
  }

  std::vector<zet_metric_group_handle_t> groups = getMetricGroups(device, num_groups);
  group = findMatchingMetricGroup(groups, metric_group_name);
  if (group == nullptr) {
    std::cerr << "[ERROR] Invalid metric group " << metric_group_name << std::endl;
    exit(-1);
  }
}

void
zeroMetricStreamerReadData
(
  zet_metric_streamer_handle_t streamer,
  std::vector<uint8_t>& storage,
  uint64_t& data_size
)
{
  ze_result_t status = ZE_RESULT_SUCCESS;
  size_t actual_data_size = 0;

  status = zetMetricStreamerReadData(streamer, UINT32_MAX, &actual_data_size, nullptr);
  level0_check_result(status, __LINE__);
  assert(actual_data_size > 0);

  if (actual_data_size > storage.size()) {
    actual_data_size = storage.size();
    std::cerr << "[WARNING] Metric samples dropped." << std::endl;
  }

  status = zetMetricStreamerReadData(streamer, UINT32_MAX, &actual_data_size, storage.data());
  level0_check_result(status, __LINE__);
  data_size = actual_data_size;
}

void
zeroMetricGroupCalculateMultipleMetricValuesExp
(
  zet_metric_group_handle_t metric_group,
  int raw_size,
  const std::vector<uint8_t>& raw_metrics,
  std::vector<uint32_t>& samples,
  std::vector<zet_typed_value_t>& metrics
)
{
  uint32_t num_samples = 0;
  uint32_t num_metrics = 0;

  // First call to get the number of samples and metrics
  ze_result_t status = zetMetricGroupCalculateMultipleMetricValuesExp(
    metric_group, ZET_METRIC_GROUP_CALCULATION_TYPE_METRIC_VALUES,
    raw_size, raw_metrics.data(), &num_samples, &num_metrics,
    nullptr, nullptr);

  if (status != ZE_RESULT_SUCCESS || num_samples == 0 || num_metrics == 0) {
    return;
  }

  // Resize vectors to accommodate the data
  samples.resize(num_samples);
  metrics.resize(num_metrics);

  // Second call to actually get the data
  status = zetMetricGroupCalculateMultipleMetricValuesExp(
    metric_group, ZET_METRIC_GROUP_CALCULATION_TYPE_METRIC_VALUES,
    raw_size, raw_metrics.data(), &num_samples, &num_metrics,
    samples.data(), metrics.data());

  if (status != ZE_RESULT_SUCCESS && status != ZE_RESULT_WARNING_DROPPED_DATA) {
    std::cerr << "[WARNING] Unable to calculate metrics" << std::endl;
    samples.clear();
    metrics.clear();
  }
}

void
zeroProcessMetrics
(
  const std::vector<std::string>& metric_list,
  const std::vector<uint32_t>& samples,
  const std::vector<zet_typed_value_t>& metrics,
  std::map<uint64_t, EuStalls>& eustalls
)
{
  const zet_typed_value_t* value = metrics.data();
  for (uint32_t i = 0; i < samples.size(); ++i) {
    uint32_t size = samples[i];
    for (uint32_t j = 0; j < size; j += metric_list.size()) {
      processMetricSample(value + j, eustalls);
    }
    value += samples[i];
  }
}
