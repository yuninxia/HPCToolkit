// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: Apache-2.0

// -*-Mode: C++;-*-

#ifndef LEVEL0_ACTIVITY_SEND_H
#define LEVEL0_ACTIVITY_SEND_H

//*****************************************************************************
// local includes
//*****************************************************************************

#include "../../../../activity/gpu-activity.h"

extern "C" {
  #include "../../../../activity/gpu-activity-channel.h"
}


//******************************************************************************
// interface operations
//******************************************************************************

void
level0SendActivities
(
  const std::deque<gpu_activity_t*>& activities
);


#endif // LEVEL0_ACTIVITY_SEND_H
