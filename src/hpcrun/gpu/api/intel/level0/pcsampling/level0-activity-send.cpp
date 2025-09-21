// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: BSD-3-Clause

// -*-Mode: C++;-*-

//*****************************************************************************
// system includes
//*****************************************************************************

#include <deque>
#include <iostream>
#include <unordered_map>
#include <pthread.h>


//*****************************************************************************
// local includes
//*****************************************************************************

#include "level0-activity-send.hpp"
#include "pcsampling-api-receiver.hpp"


//******************************************************************************
// interface operations
//******************************************************************************

void
level0SendActivities
(
  const std::deque<gpu_activity_t*>& activities
)
{
  if (activities.empty()) {
    return;
  }

  // Send activities directly using correlation ID
  // This avoids the need for the PC sampling thread to have an activity channel
  for (const auto& activity : activities) {
    if (activity) {
      uint64_t correlation_id = activity->details.pc_sampling.correlation_id;
      pcsampling::sendActivityDirect(correlation_id, activity);
    }
  }
}
