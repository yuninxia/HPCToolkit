// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: Apache-2.0
//
// SPDX-License-Identifier: MIT
// Copyright (C) 2024 Intel Corporation
// This file was inspired by and uses some code fragments from Intel's
// MIT-licensed pti-gpu (https://github.com/intel/pti-gpu)

// -*-Mode: C++;-*-

#ifndef LEVEL0_CORRELATION_ID_H_
#define LEVEL0_CORRELATION_ID_H_

//*****************************************************************************
// local includes
//*****************************************************************************

#include "level0-device.hpp"

extern "C" {
  #include "../../../../activity/correlation/gpu-correlation-channel.h"
  #include "../../../../activity/gpu-activity-channel.h"
}


//******************************************************************************
// interface operations
//******************************************************************************

void
level0UpdateCorrelationId
(
  uint64_t cid,
  gpu_activity_channel_t *channel,
  void *arg
);


#endif // LEVEL0_CORRELATION_ID_H_
