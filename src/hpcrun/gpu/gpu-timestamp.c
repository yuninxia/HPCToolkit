// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: Apache-2.0

// -*-Mode: C++;-*- // technically C99

//******************************************************************************
// system includes
//******************************************************************************

#include <threads.h>



//******************************************************************************
// hpctoolkit includes
//******************************************************************************

#include "../utilities/hpcrun-nanotime.h"
#include "gpu-timestamp.h"



//******************************************************************************
// private data
//******************************************************************************

static uint64_t clock_offset_ns;



//******************************************************************************
// debugging support
//******************************************************************************

#define DEBUG 0

#include "common/gpu-print.h"



//******************************************************************************
// private operations
//******************************************************************************

static void
clock_offset_ns_init
(
  void
)
{
  clock_offset_ns = hpcrun_nanotime_real_boot_offset();
}



//******************************************************************************
// interface operations
//******************************************************************************

uint64_t
gpu_timestamp_boottime_to_realtime
(
  uint64_t t
)
{
  static once_flag once = ONCE_FLAG_INIT;
  call_once(&once, clock_offset_ns_init);

  uint64_t result = t + clock_offset_ns;

  PRINT("gpu_timestamp_boottime_to_realtime(%ld) --> %ld\n", t, result);

  return result;
}
