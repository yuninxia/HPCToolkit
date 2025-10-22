// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: Apache-2.0
//
// SPDX-License-Identifier: MIT
// Copyright (C) 2024 Intel Corporation
// This file was inspired by and uses some code fragments from Intel's
// MIT-licensed pti-gpu (https://github.com/intel/pti-gpu)

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
