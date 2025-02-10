// SPDX-FileCopyrightText: 2002-2024 Rice University
// SPDX-FileCopyrightText: 2024 Contributors to the HPCToolkit Project
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
  assert(units != nullptr);

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
  std::vector<zet_metric_handle_t> metric_list(metric_count);
  ze_result_t status = f_zetMetricGet(group, &metric_count, metric_list.data(), dispatch);
  level0_check_result(status, __LINE__);
  // Verify that the retrieved metric count matches the vector size
  assert(metric_count == metric_list.size());
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
  assert(!metric_list.empty());
  assert(!metric_name.empty());

  for (size_t i = 0; i < metric_list.size(); ++i) {
    if (metric_list[i].find(metric_name) == 0) {
      metric_id = static_cast<uint32_t>(i);
      return;
    }
  }
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
  // Retrieve the list of metric names from the given metric group
  assert(group != nullptr);

  // Get the number of metrics in the group
  uint32_t metric_count = getMetricCount(group, dispatch);
  assert(metric_count > 0);

  // Retrieve metric handles
  std::vector<zet_metric_handle_t> metric_handles = getMetricHandles(group, metric_count, dispatch);

  // Clear the output list and populate it with formatted metric names
  name_list.clear();
  for (auto metric : metric_handles) {
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
  // Validate the metric list by checking if a metric starting with "IP" exists
  if (metric_list.empty()) return false;
  uint32_t ip_idx;
  getMetricId(metric_list, "IP", ip_idx);
  return ip_idx < metric_list.size();
}
