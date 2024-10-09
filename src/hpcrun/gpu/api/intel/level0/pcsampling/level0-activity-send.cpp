// SPDX-FileCopyrightText: 2002-2024 Rice University
// SPDX-FileCopyrightText: 2024 Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: BSD-3-Clause

// -*-Mode: C++;-*-

//*****************************************************************************
// local includes
//*****************************************************************************

#include "level0-activity-send.hpp"


//******************************************************************************
// interface operations
//******************************************************************************

void
zeroSendActivities
(
  std::deque<gpu_activity_t*>& activities
) 
{
  for (auto activity : activities) {
    uint32_t thread_id = gpu_activity_channel_correlation_id_get_thread_id(activity->details.instruction.correlation_id);
    gpu_activity_channel_t *channel = gpu_activity_channel_lookup(thread_id);
    gpu_activity_channel_send(channel, activity);
  }
}
