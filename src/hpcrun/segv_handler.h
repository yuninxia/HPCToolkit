// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: Apache-2.0

// -*-Mode: C++;-*- // technically C99

#ifndef SEGV_HANDLER_H
#define SEGV_HANDLER_H

typedef void (*hpcrun_sig_callback_t) (void);

int
hpcrun_segv_register_cb( hpcrun_sig_callback_t cb );

int
hpcrun_setup_segv();

#endif
