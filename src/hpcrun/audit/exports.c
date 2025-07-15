// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: Apache-2.0

// -*-Mode: C++;-*-

#define _GNU_SOURCE

#include "audit-api.h"

#include <unistd.h>
#include <sys/wait.h>
#include <sched.h>
#include <stdlib.h>
#include <signal.h>
#include <pthread.h>

// no-op for --disable-auditor
static void initialization_dummy(void) {};


__attribute__((visibility("default")))
void export_symbols(auditor_exports_t* exports) {
  exports->exit = exit;
  exports->sigprocmask = sigprocmask;
  exports->pthread_sigmask = pthread_sigmask;
  exports->sigaction = sigaction;
  exports->pthread_self = pthread_self;
  exports->pthread_kill = pthread_kill;
  exports->pthread_setcancelstate = pthread_setcancelstate;
  exports->getenv = getenv;
  exports->begin_initialization = initialization_dummy;
  exports->end_initialization = initialization_dummy;
}
