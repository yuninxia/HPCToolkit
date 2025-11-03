// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: Apache-2.0

// -*-Mode: C++;-*- // technically C99

#ifndef level0_timestamps_h
#define level0_timestamps_h

//*****************************************************************************
// includes
//*****************************************************************************

#include <stdint.h>
#include "../../../../foil/level0.h"



//*****************************************************************************
// interface operations
//*****************************************************************************

uint64_t
level0_convert_device_time_to_host_time
(
  ze_device_handle_t hDevice,
  const struct hpcrun_foil_appdispatch_level0 *dispatch,
  uint64_t device_time
);

#endif
