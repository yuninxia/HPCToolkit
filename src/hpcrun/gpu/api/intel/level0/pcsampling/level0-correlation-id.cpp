// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: BSD-3-Clause

// -*-Mode: C++;-*-

//*****************************************************************************
// system includes
//*****************************************************************************

//*****************************************************************************
// local includes
//*****************************************************************************

#include "level0-correlation-id.hpp"
#include "pcsampling-api-receiver.hpp"


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
