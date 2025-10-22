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
// local includes
//*****************************************************************************

#include "level0-correlation-id.hpp"
#include "level0-pc-api-receiver.hpp"


//******************************************************************************
// interface operations
//******************************************************************************

void
level0UpdateCorrelationId
(
  uint64_t cid,
  gpu_activity_channel_t *channel,
  void *arg
)
{
  if (arg == nullptr) {
    pcsampling::warn("Null device descriptor passed to level0UpdateCorrelationId");
    return;
  }

  ZeDeviceDescriptor* desc = static_cast<ZeDeviceDescriptor*>(arg);
  desc->correlation_id_ = cid;
}
