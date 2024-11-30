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
// interface operations
//******************************************************************************

void
zeroInitializeMetricStreamer
(
  ze_context_handle_t context,
  ze_device_handle_t device,
  zet_metric_group_handle_t group,
  zet_metric_streamer_handle_t& streamer
)
{
  ze_result_t status = zetContextActivateMetricGroups(context, device, 1, &group);
  level0_check_result(status, __LINE__);

  uint32_t interval = 500000; // ns
  uint32_t notifyEveryNReports = 65536;
  zet_metric_streamer_desc_t streamer_desc = { 
    ZET_STRUCTURE_TYPE_METRIC_STREAMER_DESC, 
    nullptr, 
    notifyEveryNReports, 
    interval 
  };

  status = zetMetricStreamerOpen(context, device, group, &streamer_desc, nullptr, &streamer);
  if (status != ZE_RESULT_SUCCESS) {
    std::cerr << "[ERROR] Failed to open metric streamer (" << status 
              << "). The sampling interval might be too small." << std::endl;
    return;
  }

  if (streamer_desc.notifyEveryNReports > max_metric_samples) {
    max_metric_samples = streamer_desc.notifyEveryNReports;
  }
}

void
zeroCleanupMetricStreamer
(
  ze_context_handle_t context,
  ze_device_handle_t device,
  zet_metric_group_handle_t group,
  zet_metric_streamer_handle_t streamer
)
{
  ze_result_t status = zetMetricStreamerClose(streamer);
  level0_check_result(status, __LINE__);

  status = zetContextActivateMetricGroups(context, device, 0, &group);
  level0_check_result(status, __LINE__);
}
