// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: BSD-3-Clause

// -*-Mode: C++;-*-

#ifndef LEVEL0_ACTIVITY_SEND_H
#define LEVEL0_ACTIVITY_SEND_H

//*****************************************************************************
// system includes
//*****************************************************************************

#include <deque>


//*****************************************************************************
// local includes
//*****************************************************************************

#include "../../../../activity/gpu-activity.h"
#include "level0-kernel-properties.hpp"

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