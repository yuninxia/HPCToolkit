// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: Apache-2.0
//
// SPDX-License-Identifier: MIT
// Copyright (C) 2024 Intel Corporation
// This file was inspired by and uses some code fragments from Intel's
// MIT-licensed pti-gpu (https://github.com/intel/pti-gpu)

// -*-Mode: C++;-*-

//*****************************************************************************
// system includes
//*****************************************************************************

#include <atomic>

//*****************************************************************************
// local includes
//*****************************************************************************

#include "level0-metric-streamer.hpp"
#include "level0-pc-api-receiver.hpp"


//******************************************************************************
// global variables
//******************************************************************************

std::atomic<uint32_t> max_metric_samples(65536);


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
  ze_result_t status = pcsampling::callZetContextActivateMetricGroups(context, device, count, &group, dispatch);
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

  ze_result_t status = pcsampling::callZetMetricStreamerOpen(context, device, group, &streamer_desc,
                                                            nullptr, &streamer, dispatch);
  if (status != ZE_RESULT_SUCCESS) {
    pcsampling::error("Failed to open metric streamer (%d). The sampling interval might be too small.", status);
    return;
  }

  // Update the global maximum if necessary (atomic compare-exchange)
  uint32_t current = max_metric_samples.load();
  while (streamer_desc.notifyEveryNReports > current) {
    if (max_metric_samples.compare_exchange_weak(current, streamer_desc.notifyEveryNReports)) {
      break;
    }
  }
}

static void
closeMetricStreamer
(
  zet_metric_streamer_handle_t streamer,
  const struct hpcrun_foil_appdispatch_level0* dispatch
)
{
  if (streamer == nullptr) {
    return;
  }

  ze_result_t status = pcsampling::callZetMetricStreamerClose(streamer, dispatch);
  level0_check_result(status, __LINE__);
}


//******************************************************************************
// interface operations
//******************************************************************************

void
level0InitializeMetricStreamer
(
  ze_context_handle_t context,
  ze_device_handle_t device,
  zet_metric_group_handle_t group,
  zet_metric_streamer_handle_t& streamer,
  const struct hpcrun_foil_appdispatch_level0* dispatch
)
{
  if (context == nullptr || device == nullptr || group == nullptr) {
    pcsampling::error("Invalid parameters for metric streamer initialization");
    return;
  }

  // Activate the metric group
  activateMetricGroup(context, device, group, 1, dispatch);

  // Open the metric streamer
  openMetricStreamer(context, device, group, streamer, dispatch);

  if (streamer == nullptr) {
    pcsampling::error("Failed to initialize metric streamer");
    // Revert the activation so callers do not leak state on failure.
    activateMetricGroup(context, device, group, 0, dispatch);
  }
}

void
level0CleanupMetricStreamer
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
