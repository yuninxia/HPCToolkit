// SPDX-FileCopyrightText: 2002-2024 Rice University
// SPDX-FileCopyrightText: 2024 Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: BSD-3-Clause

// -*-Mode: C++;-*-

//*****************************************************************************
// local includes
//*****************************************************************************

#include "level0-metric-streamer.hpp"


//******************************************************************************
// global variables
//******************************************************************************

uint32_t max_metric_samples = 65536;


//******************************************************************************
// private operations
//******************************************************************************

static void
activateMetricGroup
(
  ze_context_handle_t context,
  ze_device_handle_t device,
  zet_metric_group_handle_t group,
  uint32_t count,
  const struct hpcrun_foil_appdispatch_level0* dispatch
)
{
  ze_result_t status = f_zetContextActivateMetricGroups(context, device, count, &group, dispatch);
  level0_check_result(status, __LINE__);
}

static void
openMetricStreamer
(
  ze_context_handle_t context,
  ze_device_handle_t device,
  zet_metric_group_handle_t group,
  zet_metric_streamer_handle_t& streamer,
  const struct hpcrun_foil_appdispatch_level0* dispatch
)
{
  constexpr uint32_t kInterval = 500000;         // Sampling interval in nanoseconds
  constexpr uint32_t kNotifyEveryNReports = 65536; // Notification rate

  zet_metric_streamer_desc_t streamer_desc = {
    ZET_STRUCTURE_TYPE_METRIC_STREAMER_DESC,
    nullptr,
    kNotifyEveryNReports,
    kInterval
  };

  ze_result_t status = f_zetMetricStreamerOpen(context, device, group, &streamer_desc, nullptr, &streamer, dispatch);
  if (status != ZE_RESULT_SUCCESS) {
    std::cerr << "[ERROR] Failed to open metric streamer (" << status 
              << "). The sampling interval might be too small." << std::endl;
    return;
  }

  // Update the global maximum if necessary
  if (streamer_desc.notifyEveryNReports > max_metric_samples) {
    max_metric_samples = streamer_desc.notifyEveryNReports;
  }
}

static void
closeMetricStreamer
(
  zet_metric_streamer_handle_t streamer,
  const struct hpcrun_foil_appdispatch_level0* dispatch
)
{
  ze_result_t status = f_zetMetricStreamerClose(streamer, dispatch);
  level0_check_result(status, __LINE__);
}


//******************************************************************************
// interface operations
//******************************************************************************

void
zeroInitializeMetricStreamer
(
  ze_context_handle_t context,
  ze_device_handle_t device,
  zet_metric_group_handle_t group,
  zet_metric_streamer_handle_t& streamer,
  const struct hpcrun_foil_appdispatch_level0* dispatch
)
{
  // Activate the metric group
  activateMetricGroup(context, device, group, 1, dispatch);

  // Open the metric streamer
  openMetricStreamer(context, device, group, streamer, dispatch);
}

void
zeroCleanupMetricStreamer
(
  ze_context_handle_t context,
  ze_device_handle_t device,
  zet_metric_group_handle_t group,
  zet_metric_streamer_handle_t streamer,
  const struct hpcrun_foil_appdispatch_level0* dispatch
)
{
  closeMetricStreamer(streamer, dispatch);

  // Deactivate the metric group
  activateMetricGroup(context, device, group, 0, dispatch);
}
