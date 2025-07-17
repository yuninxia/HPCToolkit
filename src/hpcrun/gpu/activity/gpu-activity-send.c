// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: BSD-3-Clause

// -*-Mode: C++;-*- // technically C99


//******************************************************************************
// hpctoolkit includes
//******************************************************************************

#include "gpu-activity-channel.h"
#include "gpu-activity-send.h"

//******************************************************************************
// debugging support
//******************************************************************************

#define DEBUG 0

#include "../common/gpu-print.h"



//******************************************************************************
// interface operations
//******************************************************************************

void
gpu_activity_send
(
  uint64_t correlation_id,
  gpu_activity_t *gpu_activity
)
{
  uint32_t thread_id =
    gpu_activity_channel_correlation_id_get_thread_id(correlation_id);

  gpu_activity_channel_t *channel = gpu_activity_channel_lookup(thread_id);

  if (channel == NULL) {
    PRINT("Cannot find gpu_activity_channel "
          "(correlation ID: %" PRIu64 ")", correlation_id);
    return;
  }

  PRINT("gpu_activity_send: sending activity kind(%d)=%s to thread %d (extid=0x%lx)\n",
        gpu_activity->kind, gpu_activity_kind_to_string(gpu_activity->kind),
        thread_id, correlation_id);

  gpu_activity_channel_send(channel, gpu_activity);
}