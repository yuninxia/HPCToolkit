// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: Apache-2.0

// -*-Mode: C++;-*- // technically C99

#ifndef level0_device_properties_h
#define level0_device_properties_h

//*****************************************************************************
// includes
//*****************************************************************************

#include <stdint.h>
#include "../../../../foil/level0.h"



//*****************************************************************************
// type declarations
//*****************************************************************************

typedef struct synchronized_timestamps_t {
  uint64_t host;
  uint64_t device;
} synchronized_timestamps_t;

typedef struct level0_device_properties_t {
  ze_device_properties_t device_props;
  uint64_t device_timestamp_resolution_ns;
  synchronized_timestamps_t recent_synchronized_timestamps;
} level0_device_properties_t;



//*****************************************************************************
// interface operations
//*****************************************************************************

// return level0 device properties, which include a pair of host and device
// timestamps that are sufficiently recent to enable device timestamps to be
// converted to host timestamps
level0_device_properties_t *
level0_device_properties_get
(
 ze_device_handle_t hDevice,
 const struct hpcrun_foil_appdispatch_level0 *dispatch
);



#endif
