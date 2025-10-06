// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: Apache-2.0

// -*-Mode: C++;-*- // technically C99

//*****************************************************************************
// system includes
//*****************************************************************************

#define _GNU_SOURCE

#include <errno.h>
#include <stdlib.h>
#include <time.h>

//*****************************************************************************
// local includes
//*****************************************************************************

#include "hpcrun-nanotime.h"

#include "../messages/errors.h"


//*****************************************************************************
// macros
//*****************************************************************************

#define NS_PER_SEC 1000000000


//*****************************************************************************
//*****************************************************************************
// interface operations
//*****************************************************************************

uint64_t
hpcrun_nanotime_clock
(
  clockid_t clock
)
{
  struct timespec now;

  int res = clock_gettime(clock, &now);
  if (res != 0)
    hpcrun_terminate();  // clock_gettime failed!

  uint64_t now_sec = now.tv_sec;
  uint64_t now_ns = now_sec * NS_PER_SEC + now.tv_nsec;

  return now_ns;
}


uint64_t
hpcrun_nanotime_real_boot_offset
(
  void
)
{
  uint64_t real = hpcrun_nanotime_clock(CLOCK_REALTIME);
  uint64_t boot = hpcrun_nanotime_clock(CLOCK_BOOTTIME);
  return real - boot;;
}


uint64_t
hpcrun_nanotime
(
  void
)
{
  return hpcrun_nanotime_clock(CLOCK_REALTIME);
}


int32_t
hpcrun_nanosleep
(
  uint32_t nsec
)
{
  struct timespec time_wait = {.tv_sec=0, .tv_nsec=nsec};
  struct timespec time_rem = {.tv_sec=0, .tv_nsec=0};
  int32_t ret;

  for(;;){
    ret = nanosleep(&time_wait, &time_rem);
    if (! (ret < 0 && errno == EINTR)){
      // normal non-signal return
      break;
    }
  time_wait = time_rem;
  }

  return ret;
}
