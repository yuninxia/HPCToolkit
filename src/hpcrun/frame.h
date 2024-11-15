// SPDX-FileCopyrightText: 2002-2024 Rice University
// SPDX-FileCopyrightText: 2024 Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: BSD-3-Clause

// -*-Mode: C++;-*- // technically C99

#ifndef FRAME_H
#define FRAME_H

#include "unwind/common/unw-datatypes.h"
#include "utilities/ip-normalized.h"
#include "utilities/ip-normalized.h"

// --------------------------------------------------------------------------
// frame_t: similar to cct_node_t, but specialized for the backtrace buffer
// --------------------------------------------------------------------------

typedef struct frame_t {
  hpcrun_unw_cursor_t cursor;       // hold a copy of the cursor for this frame
  ip_normalized_t ip_norm;
  ip_normalized_t the_function;     // enclosing function of ip_norm
  void* ra_loc;
  void* ra_val;
} frame_t;

static inline
void*
hpcrun_frame_get_unnorm(frame_t* frame)
{
  return frame->cursor.pc_unnorm;
}

#endif // FRAME_H
