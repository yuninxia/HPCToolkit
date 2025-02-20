// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: BSD-3-Clause

// -*-Mode: C++;-*- // technically C99

#ifndef x86_addsub_h
#define x86_addsub_h

#include "x86-unwind-analysis.h"
#include "x86-interval-arg.h"

unwind_interval *
process_addsub(xed_decoded_inst_t *xptr, const xed_inst_t *xi, interval_arg_t *iarg);

#endif
