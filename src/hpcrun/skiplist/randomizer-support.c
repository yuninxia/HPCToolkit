// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: BSD-3-Clause

// -*-Mode: C++;-*- // technically C99

//----------------------------------------------------------------------

// This file provides stubs for some functions used by the randomizer
// test.  Using the real (hpcrun) versions pulls in the cat who killed
// the rat that ate the malt that pulls in all of hpcrun.

//----------------------------------------------------------------------

#include <sys/types.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>

void
messages_donothing(void)
{
}

void
hpcrun_abort_w_info(void (*info)(void), const char *fmt, ...)
{
  exit(1);
}

void *
hpcrun_ompt_alloc_specific(void)
{
  return NULL;
}
