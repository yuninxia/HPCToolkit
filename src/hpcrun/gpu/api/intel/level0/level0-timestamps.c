// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: Apache-2.0

// -*-Mode: C++;-*- // technically C99

//*****************************************************************************
// include files
//*****************************************************************************

#include "level0-device-properties.h"
#include "level0-timestamps.h"



///******************************************************************************
// debugging support
//******************************************************************************

#define DEBUG 0
#include "../../../common/gpu-print.h"



//*****************************************************************************
// private operations
//*****************************************************************************

/// @brief Mask off bits above lowest device_time_nbits
static uint64_t
extract_device_time
(
  uint64_t device_time,
  uint64_t device_time_nbits
)
{
  uint64_t mask = (~0ULL) >> (64 - device_time_nbits);
  device_time &= mask;
  return device_time;
}


/// @brief Compute the offset between current and base times. Consider only valid bits.
static int64_t
compute_device_time_offset
(
  uint64_t base,
  uint64_t event,
  uint64_t device_time_nbits
)
{
  // limit time focus to valid bits
  base = extract_device_time(base, device_time_nbits);
  event = extract_device_time(event, device_time_nbits);

#if 0
  // useful model code to compute the difference between an event time
  // and a base time. this properly accounts for a timer wrap under the
  // assumption that the event time is later than the base time. however,
  // we can't assume that event time is later than base time. grr.
  uint64_t offset = event - base;

  if (event < base) {
    // handle single device timer overflow
    offset += 1ULL << device_time_nbits;
  }
#else
  // we can't handle timer overflow because we are computing a difference
  // between a "recent" device time and the current device time. however,
  // the strategy of polling the device periodically to obtain "recent"
  // (host, device) pairs may mean that the recent time may be on either
  // side of the event time. unless we poll for a new pair, we won't know
  // for sure which is earlier. so, we just assume that the device timer
  // didn't wrap. sigh.
  int64_t offset = event - base;
#endif

  return offset;
}



//*****************************************************************************
// interface operations
//*****************************************************************************

/// @brief Use a recent pair of synchronized (host, device) timestamps to compute the corresponding host time for device_time
uint64_t
level0_convert_device_time_to_host_time
(
  ze_device_handle_t hDevice,
  const struct hpcrun_foil_appdispatch_level0 *dispatch,
  uint64_t device_time
)
{
  level0_device_properties_t *properties = level0_device_properties_get(hDevice, dispatch);

  int64_t device_cycles_elapsed_since_sync =
    compute_device_time_offset(properties->recent_synchronized_timestamps.device, device_time,
      properties->device_props.kernelTimestampValidBits);

  uint64_t device_ns_elapsed_since_sync = device_cycles_elapsed_since_sync * properties->device_timestamp_resolution_ns;
  uint64_t host_time_for_device_time = properties->recent_synchronized_timestamps.host + device_ns_elapsed_since_sync;

  PRINT("level0_convert_device_time_to_host_time: (h=%ld, d=%ld) in=%ld out=%ld\n",
    properties->recent_synchronized_timestamps.host,
    properties->recent_synchronized_timestamps.device,
    device_time, host_time_for_device_time);

  return host_time_for_device_time;
}
