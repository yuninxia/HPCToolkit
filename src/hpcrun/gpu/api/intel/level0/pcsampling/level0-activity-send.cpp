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
  for (auto* activity : activities) {
    if (!activity) {
      continue;
    }

    pcsampling::sendActivityDirect(activity->details.instruction.correlation_id,
                                   activity);
  }
}
