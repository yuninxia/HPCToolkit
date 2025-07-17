// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: Apache-2.0

// -*-Mode: C++;-*- // technically C99

// ******************************************************************************
// System Includes
// ******************************************************************************

#define _GNU_SOURCE

#include <stdlib.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <ucontext.h>

// ******************************************************************************
// Local Includes
// ******************************************************************************

#include "../mcontext.h"
#include "../context-pc.h"

//***************************************************************************
// interface functions
//***************************************************************************


void *
hpcrun_context_pc_async(void *context)
{
  ucontext_t* ctxt = (ucontext_t*)context;
  return ucontext_pc(ctxt);
}
