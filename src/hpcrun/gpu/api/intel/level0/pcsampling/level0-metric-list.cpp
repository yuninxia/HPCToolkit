// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: BSD-3-Clause

// -*-Mode: C++;-*-

//*****************************************************************************
// system includes
//*****************************************************************************

//*****************************************************************************
// local includes
//*****************************************************************************

#include "level0-metric-list.hpp"
#include "level0-pc-api-receiver.hpp"


//******************************************************************************
// private operations
//******************************************************************************

static void
getMetricUnits
(
  const char* units,
  std::string& result
)
{
  if (units == nullptr) {
    result = "";
    return;
  }

  result = units;
  if (result.find("null") != std::string::npos) {
    result = "";
  } else if (result.find("percent") != std::string::npos) {
    result = "%";
  }
}

static uint32_t
getMetricCount
(
  zet_metric_group_handle_t group,
  const struct hpcrun_foil_appdispatch_level0* dispatch
)
{
  if (group == nullptr) {
    pcsampling::error("Null metric group handle passed to getMetricCount");
    return 0;
  }

  zet_metric_group_properties_t group_props{};
  group_props.stype = ZET_STRUCTURE_TYPE_METRIC_GROUP_PROPERTIES;

  ze_result_t status = pcsampling::callZetMetricGroupGetProperties(group, &group_props, dispatch);
  level0_check_result(status, __LINE__);
  return group_props.metricCount;
}

static std::vector<zet_metric_handle_t>
getMetricHandles
(
  zet_metric_group_handle_t group,
  uint32_t metric_count,
  const struct hpcrun_foil_appdispatch_level0* dispatch
)
{
  if (group == nullptr) {
    pcsampling::error("Null metric group handle passed to getMetricHandles");
    return {};
  }

  if (metric_count == 0) {
    pcsampling::warn("Zero metric count passed to getMetricHandles");
    return {};
  }

  std::vector<zet_metric_handle_t> metric_list(metric_count);
  ze_result_t status = pcsampling::callZetMetricGet(group, &metric_count, metric_list.data(), dispatch);
  level0_check_result(status, __LINE__);
  // Verify that the retrieved metric count matches the vector size
  if (metric_count != metric_list.size()) {
    pcsampling::warn("Metric count mismatch: expected %zu, got %u", metric_list.size(), metric_count);
    // Resize the vector to match the actual count
    metric_list.resize(metric_count);
  }

  return metric_list;
}

static zet_metric_properties_t
getMetricProperties
(
  zet_metric_handle_t metric,
  const struct hpcrun_foil_appdispatch_level0* dispatch
)
{
  zet_metric_properties_t metric_props{ZET_STRUCTURE_TYPE_METRIC_PROPERTIES};

  if (metric == nullptr) {
    pcsampling::error("Null metric handle passed to getMetricProperties");
    return metric_props;
  }

  ze_result_t status = pcsampling::callZetMetricGetProperties(metric, &metric_props, dispatch);
  level0_check_result(status, __LINE__);
  return metric_props;
}

static std::string
buildMetricName
(
  const zet_metric_properties_t& metric_props
)
{
  std::string units;
  getMetricUnits(metric_props.resultUnits, units);

  std::string name = metric_props.name;
  if (!units.empty()) {
    name += "[" + units + "]";
  }
  return name;
}

static void
getMetricId
(
  const std::vector<std::string>& metric_list,
  const std::string& metric_name,
  uint32_t& metric_id
)
{
  // Default to an invalid ID
  metric_id = static_cast<uint32_t>(-1);

  if (metric_list.empty()) {
    pcsampling::warn("Empty metric list passed to getMetricId");
    return;
  }

  if (metric_name.empty()) {
    pcsampling::warn("Empty metric name passed to getMetricId");
    return;
  }

  for (size_t i = 0; i < metric_list.size(); ++i) {
    if (metric_list[i].find(metric_name) == 0) {
      metric_id = static_cast<uint32_t>(i);
      return;
    }
  }

  // If we get here, the metric was not found
  pcsampling::warn("Metric '%s' not found in metric list", metric_name.c_str());
  metric_id = static_cast<uint32_t>(metric_list.size());
}


//******************************************************************************
// interface operations
//******************************************************************************

void
level0GetMetricList
(
  zet_metric_group_handle_t group,
  std::vector<std::string>& name_list,
  const struct hpcrun_foil_appdispatch_level0* dispatch
)
{
  // Clear the output list before populating it
  name_list.clear();

  if (group == nullptr) {
    pcsampling::error("Null metric group handle passed to level0GetMetricList");
    return;
  }

  // Get the number of metrics in the group
  uint32_t metric_count = getMetricCount(group, dispatch);
  if (metric_count == 0) {
    pcsampling::warn("No metrics found in the metric group");
    return;
  }

  // Retrieve metric handles
  std::vector<zet_metric_handle_t> metric_handles = getMetricHandles(group, metric_count, dispatch);
  if (metric_handles.empty()) {
    pcsampling::warn("Failed to retrieve metric handles");
    return;
  }

  // Populate the output list with formatted metric names
  for (auto metric : metric_handles) {
    if (metric == nullptr) {
      pcsampling::warn("Null metric handle encountered");
      continue;
    }

    zet_metric_properties_t metric_props = getMetricProperties(metric, dispatch);
    std::string name = buildMetricName(metric_props);
    name_list.push_back(std::move(name));
  }
}

bool
level0IsValidMetricList
(
  const std::vector<std::string>& metric_list
)
{
  if (metric_list.empty()) {
    pcsampling::warn("Empty metric list passed to level0IsValidMetricList");
    return false;
  }

  uint32_t ip_idx;
  getMetricId(metric_list, "IP", ip_idx);

  bool is_valid = (ip_idx < metric_list.size());
  if (!is_valid) {
    pcsampling::warn("No 'IP' metric found in the metric list");
  }

  return is_valid;
}
