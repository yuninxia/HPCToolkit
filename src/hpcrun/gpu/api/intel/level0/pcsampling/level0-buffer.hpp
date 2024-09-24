// SPDX-FileCopyrightText: 2002-2024 Rice University
// SPDX-FileCopyrightText: 2024 Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: BSD-3-Clause

// -*-Mode: C++;-*-

#ifndef LEVEL0_BUFFER_H_
#define LEVEL0_BUFFER_H_

//*****************************************************************************
// level zero includes
//*****************************************************************************

#include <level_zero/ze_api.h>
#include <level_zero/zet_api.h>


//*****************************************************************************
// local includes
//*****************************************************************************

#include "level0-device.hpp"


//*****************************************************************************
// global variables
//*****************************************************************************

extern uint32_t max_metric_samples;


//******************************************************************************
// interface operations
//******************************************************************************

void
zeroFlushStreamerBuffer
(
  zet_metric_streamer_handle_t& streamer,
  ZeDeviceDescriptor* desc
);


#endif // LEVEL0_BUFFER_H_