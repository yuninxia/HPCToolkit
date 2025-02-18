// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: BSD-3-Clause

#define _GNU_SOURCE

#include <ucontext.h>

//***************************************************************************
// local include files
//***************************************************************************

#include "../context-pc.h"
#include "../mcontext.h"

//****************************************************************************
// interface functions
//****************************************************************************

void*
hpcrun_context_pc_async(void* context)
{
  return ucontext_pc(context);
}
