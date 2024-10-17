// SPDX-FileCopyrightText: 2002-2024 Rice University
// SPDX-FileCopyrightText: 2024 Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: BSD-3-Clause

//*****************************************************************************
// local includes
//*****************************************************************************

#include "level0-streamer-buffer.hpp"
#include "level0-assert.hpp"


//******************************************************************************
// interface operations
//******************************************************************************

void
zeroFlushStreamerBuffer
(
  zet_metric_streamer_handle_t& streamer,
  ZeDeviceDescriptor* desc
)
{
  ze_result_t status = ZE_RESULT_SUCCESS;

  // Close the old streamer
  status = zetMetricStreamerClose(streamer);
  level0_check_result(status, __LINE__);

  // Open a new streamer
  uint32_t interval = 500000; // ns
  zet_metric_streamer_desc_t streamer_desc = {ZET_STRUCTURE_TYPE_METRIC_STREAMER_DESC, nullptr, max_metric_samples, interval};
  status = zetMetricStreamerOpen(desc->context_, desc->device_, desc->metric_group_, &streamer_desc, nullptr, &streamer);
  if (status != ZE_RESULT_SUCCESS) {
    std::cerr << "[ERROR] Failed to open metric streamer (" << status << "). The sampling interval might be too small." << std::endl;
    streamer = nullptr; // Make sure to set streamer to nullptr if fails
    return;
  }

  if (streamer_desc.notifyEveryNReports > max_metric_samples) {
    max_metric_samples = streamer_desc.notifyEveryNReports;
  }
}
