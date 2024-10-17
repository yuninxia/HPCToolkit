// SPDX-FileCopyrightText: 2002-2024 Rice University
// SPDX-FileCopyrightText: 2024 Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: BSD-3-Clause

// -*-Mode: C++;-*-

#ifndef LEVEL0_CORRELATION_ID_H_
#define LEVEL0_CORRELATION_ID_H_

//*****************************************************************************
// local includes
//*****************************************************************************

#include "level0-device.hpp"

extern "C" {
  #include "../../../../activity/correlation/gpu-correlation-channel.h"
  #include "../../../../activity/gpu-activity-channel.h"
}


//******************************************************************************
// interface operations
//******************************************************************************

void
zeroUpdateCorrelationId
(
  uint64_t cid,
  gpu_activity_channel_t *channel,
  void *arg
);


#endif // LEVEL0_CORRELATION_ID_H_