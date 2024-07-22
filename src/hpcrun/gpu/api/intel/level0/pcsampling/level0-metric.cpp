#include "level0-metric.h"

void
zeroGetMetricUnits
(
  const char* units,
  std::string& result
)
{
  PTI_ASSERT(units != nullptr);

  result = units;
  if (result.find("null") != std::string::npos) {
    result = "";
  } else if (result.find("percent") != std::string::npos) {
    result = "%";
  }
}

void
zeroGetMetricId
(
  const std::vector<std::string>& metric_list,
  const std::string& metric_name,
  uint32_t& metric_id
)
{
  PTI_ASSERT(!metric_list.empty());
  PTI_ASSERT(!metric_name.empty());

  for (size_t i = 0; i < metric_list.size(); ++i) {
    if (metric_list[i].find(metric_name) == 0) {
      metric_id = i;
      return;
    }
  }
  metric_id = metric_list.size();
}

void
zeroGetMetricCount
(
  zet_metric_group_handle_t group,
  uint32_t& metric_count
)
{
  PTI_ASSERT(group != nullptr);

  zet_metric_group_properties_t group_props{};
  group_props.stype = ZET_STRUCTURE_TYPE_METRIC_GROUP_PROPERTIES;
  ze_result_t status = zetMetricGroupGetProperties(group, &group_props);
  PTI_ASSERT(status == ZE_RESULT_SUCCESS);

  metric_count = group_props.metricCount;
}

void
zeroGetMetricList
(
  zet_metric_group_handle_t group,
  std::vector<std::string>& name_list
)
{
  PTI_ASSERT(group != nullptr);

  uint32_t metric_count;
  zeroGetMetricCount(group, metric_count);
  PTI_ASSERT(metric_count > 0);

  std::vector<zet_metric_handle_t> metric_list(metric_count);
  ze_result_t status = zetMetricGet(group, &metric_count, metric_list.data());
  PTI_ASSERT(status == ZE_RESULT_SUCCESS);
  PTI_ASSERT(metric_count == metric_list.size());

  name_list.clear();
  for (auto metric : metric_list) {
    zet_metric_properties_t metric_props{
      ZET_STRUCTURE_TYPE_METRIC_PROPERTIES,
    };
    status = zetMetricGetProperties(metric, &metric_props);
    PTI_ASSERT(status == ZE_RESULT_SUCCESS);

    std::string units;
    zeroGetMetricUnits(metric_props.resultUnits, units);

    std::string name = metric_props.name;
    if (!units.empty()) {
      name += "[" + units + "]";
    }
    name_list.push_back(name);
  }
}

void
zeroGetMetricGroup
(
  ze_device_handle_t device,
  const std::string& metric_group_name,
  zet_metric_group_handle_t& group
)
{
  uint32_t num_groups = 0;
  ze_result_t status = zetMetricGroupGet(device, &num_groups, nullptr);
  PTI_ASSERT(status == ZE_RESULT_SUCCESS);

  if (num_groups == 0) {
    std::cerr << "[WARNING] No metric groups found" << std::endl;
    group = nullptr;
    return;
  }

  std::vector<zet_metric_group_handle_t> groups(num_groups, nullptr);
  status = zetMetricGroupGet(device, &num_groups, groups.data());
  PTI_ASSERT(status == ZE_RESULT_SUCCESS);

  group = nullptr;
  for (uint32_t k = 0; k < num_groups; ++k) {
    zet_metric_group_properties_t group_props{};
    group_props.stype = ZET_STRUCTURE_TYPE_METRIC_GROUP_PROPERTIES;
    status = zetMetricGroupGetProperties(groups[k], &group_props);
    PTI_ASSERT(status == ZE_RESULT_SUCCESS);

    if ((group_props.name == metric_group_name) && (group_props.samplingType & ZET_METRIC_GROUP_SAMPLING_TYPE_FLAG_TIME_BASED)) {
        group = groups[k];
        return;
    }
  }

  if (group == nullptr) {
    std::cerr << "[ERROR] Invalid metric group " << metric_group_name << std::endl;
    exit(-1);
  }
}

void
zeroCollectMetrics
(
  zet_metric_streamer_handle_t streamer,
  std::vector<uint8_t>& storage,
  uint64_t& data_size
)
{
  ze_result_t status = ZE_RESULT_SUCCESS;
  size_t actual_data_size = 0;

  status = zetMetricStreamerReadData(streamer, UINT32_MAX, &actual_data_size, nullptr);
  PTI_ASSERT(status == ZE_RESULT_SUCCESS);
  PTI_ASSERT(actual_data_size > 0);

  if (actual_data_size > storage.size()) {
    actual_data_size = storage.size();
    std::cerr << "[WARNING] Metric samples dropped." << std::endl;
  }

  status = zetMetricStreamerReadData(streamer, UINT32_MAX, &actual_data_size, storage.data());
  PTI_ASSERT(status == ZE_RESULT_SUCCESS);
  data_size = actual_data_size;
}

void
zeroCalculateEuStalls
(
  zet_metric_group_handle_t metric_group,
  int raw_size,
  const std::vector<uint8_t>& raw_metrics,
  std::map<uint64_t, EuStalls>& eustalls
)
{
  eustalls.clear();

  std::vector<std::string> metric_list;
  zeroGetMetricList(metric_group, metric_list);
  PTI_ASSERT(!metric_list.empty());

  uint32_t ip_idx;
  zeroGetMetricId(metric_list, "IP", ip_idx);
  if (ip_idx >= metric_list.size()) {
    return; // no IP metric
  }

  uint32_t num_samples = 0;
  uint32_t num_metrics = 0;

  ze_result_t status = zetMetricGroupCalculateMultipleMetricValuesExp(
    metric_group, ZET_METRIC_GROUP_CALCULATION_TYPE_METRIC_VALUES,
    raw_size, raw_metrics.data(), &num_samples, &num_metrics,
    nullptr, nullptr);

  if (status != ZE_RESULT_SUCCESS || num_samples == 0 || num_metrics == 0) {
    std::cerr << "[WARNING] Unable to calculate metrics. ";
    if (status != ZE_RESULT_SUCCESS) {
      std::cerr << "Status: " << status << ".";
    }
    if (num_samples == 0) {
      std::cerr << "No samples collected.";
    }
    if (num_metrics == 0) {
      std::cerr << "No metrics calculated.";
    }
    std::cerr << std::endl;
    return;
  }

  std::vector<uint32_t> samples(num_samples);
  std::vector<zet_typed_value_t> metrics(num_metrics);
  status = zetMetricGroupCalculateMultipleMetricValuesExp(
    metric_group, ZET_METRIC_GROUP_CALCULATION_TYPE_METRIC_VALUES,
    raw_size, raw_metrics.data(), &num_samples, &num_metrics,
    samples.data(), metrics.data());

  if (status != ZE_RESULT_SUCCESS && status != ZE_RESULT_WARNING_DROPPED_DATA) {
    std::cerr << "[WARNING] Unable to calculate metrics" << std::endl;
    return;
  }

  const zet_typed_value_t *value = metrics.data();

  for (uint32_t i = 0; i < num_samples; ++i) {
    uint32_t size = samples[i];
    for (uint32_t j = 0; j < size; j += metric_list.size()) {
      uint64_t ip = (value[j + 0].value.ui64 << 3);
      if (ip == 0) {
        continue;
      }
      EuStalls stall;
      stall.active_ = value[j + 1].value.ui64;
      stall.control_ = value[j + 2].value.ui64;
      stall.pipe_ = value[j + 3].value.ui64;
      stall.send_ = value[j + 4].value.ui64;
      stall.dist_ = value[j + 5].value.ui64;
      stall.sbid_ = value[j + 6].value.ui64;
      stall.sync_ = value[j + 7].value.ui64;
      stall.insfetch_ = value[j + 8].value.ui64;
      stall.other_ = value[j + 9].value.ui64;
      auto [it, inserted] = eustalls.try_emplace(ip, stall);
      if (!inserted) {
        EuStalls& existing = it->second;
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
    }
    value += samples[i];
  }
}
