// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: BSD-3-Clause

// -*-Mode: C++;-*-

//*****************************************************************************
// local includes
//*****************************************************************************

#include "level0-metric-list.hpp"


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
    std::cerr << "[ERROR] Null metric group handle passed to getMetricCount" << std::endl;
    return 0;
  }

  zet_metric_group_properties_t group_props{};
  group_props.stype = ZET_STRUCTURE_TYPE_METRIC_GROUP_PROPERTIES;
  
  ze_result_t status = f_zetMetricGroupGetProperties(group, &group_props, dispatch);
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
    std::cerr << "[ERROR] Null metric group handle passed to getMetricHandles" << std::endl;
    return {};
  }

  if (metric_count == 0) {
    std::cerr << "[WARNING] Zero metric count passed to getMetricHandles" << std::endl;
    return {};
  }

  std::vector<zet_metric_handle_t> metric_list(metric_count);
  ze_result_t status = f_zetMetricGet(group, &metric_count, metric_list.data(), dispatch);
  level0_check_result(status, __LINE__);
  // Verify that the retrieved metric count matches the vector size
  if (metric_count != metric_list.size()) {
    std::cerr << "[WARNING] Metric count mismatch: expected " << metric_list.size() 
              << ", got " << metric_count << std::endl;
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
    std::cerr << "[ERROR] Null metric handle passed to getMetricProperties" << std::endl;
    return metric_props;
  }
  
  ze_result_t status = f_zetMetricGetProperties(metric, &metric_props, dispatch);
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
    std::cerr << "[WARNING] Empty metric list passed to getMetricId" << std::endl;
    return;
  }

  if (metric_name.empty()) {
    std::cerr << "[WARNING] Empty metric name passed to getMetricId" << std::endl;
    return;
  }

  for (size_t i = 0; i < metric_list.size(); ++i) {
    if (metric_list[i].find(metric_name) == 0) {
      metric_id = static_cast<uint32_t>(i);
      return;
    }
  }
  
  // If we get here, the metric was not found
  std::cerr << "[WARNING] Metric '" << metric_name << "' not found in metric list" << std::endl;
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
    std::cerr << "[ERROR] Null metric group handle passed to level0GetMetricList" << std::endl;
    return;
  }

  // Get the number of metrics in the group
  uint32_t metric_count = getMetricCount(group, dispatch);
  if (metric_count == 0) {
    std::cerr << "[WARNING] No metrics found in the metric group" << std::endl;
    return;
  }

  // Retrieve metric handles
  std::vector<zet_metric_handle_t> metric_handles = getMetricHandles(group, metric_count, dispatch);
  if (metric_handles.empty()) {
    std::cerr << "[WARNING] Failed to retrieve metric handles" << std::endl;
    return;
  }

  // Populate the output list with formatted metric names
  for (auto metric : metric_handles) {
    if (metric == nullptr) {
      std::cerr << "[WARNING] Null metric handle encountered" << std::endl;
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
    std::cerr << "[WARNING] Empty metric list passed to level0IsValidMetricList" << std::endl;
    return false;
  }
  
  uint32_t ip_idx;
  getMetricId(metric_list, "IP", ip_idx);
  
  bool is_valid = (ip_idx < metric_list.size());
  if (!is_valid) {
    std::cerr << "[WARNING] No 'IP' metric found in the metric list" << std::endl;
  }
  
  return is_valid;
}
