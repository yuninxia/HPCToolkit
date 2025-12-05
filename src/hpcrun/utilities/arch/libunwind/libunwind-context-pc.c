// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: Apache-2.0

// -*-Mode: C++;-*-

#define _GNU_SOURCE

#define UNW_LOCAL_ONLY

#include <sys/types.h>
#include <libunwind.h>
#include <stddef.h>

void *
hpcrun_context_pc_async(void *context)
{
  // We would need to start up libunwind to do this, and that can't be
  // async-signal-safe since libunwind needs to malloc internal data.
  return NULL;
}
