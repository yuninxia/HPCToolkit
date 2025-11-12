// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: Apache-2.0
//
// SPDX-License-Identifier: MIT
// Copyright (C) 2024 Intel Corporation
// This file was inspired by and uses some code fragments from Intel's
// MIT-licensed pti-gpu (https://github.com/intel/pti-gpu)

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
#include "level0-pc-api-receiver.hpp"


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
