// SPDX-FileCopyrightText: 2002-2024 Rice University
// SPDX-FileCopyrightText: 2024 Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: BSD-3-Clause

// -*-Mode: C++;-*-

#ifndef LEVEL0_METRIC_STREAMER_HPP
#define LEVEL0_METRIC_STREAMER_HPP

//*****************************************************************************
// level zero includes
//*****************************************************************************

#include <level_zero/ze_api.h>
#include <level_zero/zet_api.h>


//*****************************************************************************
// system includes
//*****************************************************************************

#include <iostream>

//*****************************************************************************
// local includes
//*****************************************************************************

#include "../../../../../foil/level0.h"
#include "level0-assert.hpp"


//******************************************************************************
// global variables
//******************************************************************************

extern uint32_t max_metric_samples;


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
);

void
level0CleanupMetricStreamer
(
  ze_context_handle_t context,
  ze_device_handle_t device,
  zet_metric_group_handle_t group,
  zet_metric_streamer_handle_t streamer,
  const struct hpcrun_foil_appdispatch_level0* dispatch
);


#endif // LEVEL0_METRIC_STREAMER_HPP