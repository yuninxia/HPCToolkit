// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: Apache-2.0

// -*-Mode: C++;-*- // technically C99

#ifndef __hpcrun_nanotime_h__
#define __hpcrun_nanotime_h__

//*****************************************************************************
// system includes
//*****************************************************************************

#include <stdint.h>



//*****************************************************************************
// interface operations
//*****************************************************************************

uint64_t
hpcrun_nanotime
(
  void
);


int32_t
hpcrun_nanosleep
(
  uint32_t nsec
);


uint64_t
hpcrun_nanotime_clock
(
  clockid_t clock
);


uint64_t
hpcrun_nanotime_real_boot_offset
(
  void
);

#endif
