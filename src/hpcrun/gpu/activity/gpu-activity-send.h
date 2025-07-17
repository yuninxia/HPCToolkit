// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: Apache-2.0

// -*-Mode: C++;-*- // technically C99

#ifndef gpu_activity_send_h
#define gpu_activity_send_h

//******************************************************************************
// system includes
//******************************************************************************

#include <stdint.h>



//******************************************************************************
// local includes
//******************************************************************************

#include "gpu-activity.h"



//******************************************************************************
// interface operations
//******************************************************************************

void
gpu_activity_send
(
  uint64_t correlation_id,
  gpu_activity_t *gpu_activity
);

#endif
