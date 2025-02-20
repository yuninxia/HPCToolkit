// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: BSD-3-Clause

// -*-Mode: C++;-*- // technically C99

#ifndef hpcrun_signals_h
#define hpcrun_signals_h

#include <sys/types.h>
#include <signal.h>

void hpcrun_signals_init(void);

int hpcrun_block_profile_signal(sigset_t * oldset);
int hpcrun_block_shootdown_signal(sigset_t * oldset);
int hpcrun_restore_sigmask(sigset_t * oldset);

void hpcrun_drain_signal(int sig);

#endif
