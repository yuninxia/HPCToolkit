// SPDX-FileCopyrightText: 2002-2024 Rice University
// SPDX-FileCopyrightText: 2024 Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: BSD-3-Clause

// -*-Mode: C++;-*-

//*****************************************************************************
// local includes
//*****************************************************************************

#include "level0-correlation-id.hpp"


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
  ZeDeviceDescriptor* desc = static_cast<ZeDeviceDescriptor*>(arg);
  desc->correlation_id_ = cid;
}