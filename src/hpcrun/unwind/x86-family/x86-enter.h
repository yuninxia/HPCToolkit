// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: BSD-3-Clause

// -*-Mode: C++;-*- // technically C99

#ifndef x86_enter_h
#define x86_enter_h

/******************************************************************************
 * XED include files
 *****************************************************************************/

#if __has_include(<xed/xed-interface.h>)
#include <xed/xed-interface.h>
#else
#include <xed-interface.h>
#endif


/******************************************************************************
 * include files
 *****************************************************************************/

#include "x86-unwind-interval.h"
#include "x86-unwind-analysis.h"
#include "x86-interval-arg.h"


/******************************************************************************
 * interface operations
 *****************************************************************************/

unwind_interval*
process_enter(xed_decoded_inst_t *xptr, const xed_inst_t *xi, interval_arg_t *iarg);

#endif
