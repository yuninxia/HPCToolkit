// SPDX-FileCopyrightText: 2002-2024 Rice University
// SPDX-FileCopyrightText: 2024 Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: BSD-3-Clause

// -*-Mode: C++;-*-

#ifndef LEVEL0_ACTIVITY_SEND_H
#define LEVEL0_ACTIVITY_SEND_H

//*****************************************************************************
// system includes
//*****************************************************************************

#include <deque>
#include <unordered_map>


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
zeroSendActivities
(
  std::deque<gpu_activity_t*>& activities
);

void
zeroLogActivities
(
  const std::deque<gpu_activity_t*>& activities, 
  const std::map<uint64_t, KernelProperties>& kprops
);


#endif // LEVEL0_ACTIVITY_SEND_H