// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: BSD-3-Clause

// -*-Mode: C++;-*- // technically C99

//***************************************************************************
// system includes
//***************************************************************************

#define _GNU_SOURCE
#include <dlfcn.h>  // for dlopen, dlsym, and RTLD_LAZY

#include <stdatomic.h> // for atomic_compare_exchange_strong
#include <stdlib.h> // for exit and getenv
#include <stdio.h>  // for fprintf
#include <string.h> // for strcmp



//***************************************************************************
// hpctoolkit includes
//***************************************************************************

#include "audit-api.h"



//***************************************************************************
// type declarations
//***************************************************************************

typedef char *(*getenv_t)(const char *);

typedef enum {
  STATE_BEFORE = 0,            // before libhpcrun.so initialization
  STATE_DURING = 1,            // during libhpcrun.so initialization
  STATE_AFTER = 2              // after libhpcrun.so initialization
} hpcrun_init_status_t;



//***************************************************************************
// local variables
//***************************************************************************

// state machine used to track program execution progress
static hpcrun_init_status_t hpcrun_init_status = STATE_BEFORE;

// pointer to outermost visible getenv. this may be an implementation of
// getenv in the executable or one in a preloaded library. or, if neither
// exposes an implementation of getenv, it might be libc getenv.
atomic_uintptr_t outermost_visible_getenv_fn = 0;

static bool verbose = false;



//***************************************************************************
// forward declarations
//***************************************************************************

extern const auditor_exports_t* hpcrun_connect_to_auditor_p();



//***************************************************************************
// private operations
//***************************************************************************

// record the outermost binding of getenv visible to the executable. if a
// preloaded library defines getenv, return that; else, if the executable
// defines getenv, return that a pointer to that; else
// return  a pointer to glibc's getenv.
static void
memoize_first_visible_getenv
(
  uintptr_t visible_getenv
)
{
  if (outermost_visible_getenv_fn) return;

  uintptr_t zero = 0; // changed to old on failure
  atomic_compare_exchange_strong(&outermost_visible_getenv_fn, &zero, visible_getenv);

   if (verbose) {
    fprintf(stderr, "memoize_first_visible_getenv: state %d outermost = %p libc = %p\n",
      hpcrun_init_status, (void *) outermost_visible_getenv_fn, &getenv);
  }
}


static char *
auditor_getenv
(
  const char *key
)
{
  static __thread int in_auditor_getenv = 0; // > 0 means recursive invocation

  static getenv_t libc_getenv_fn = &getenv;

  // (1) during libhpcrun.so initialization, use glibc getenv
  // (2) if in_auditor_getenv > 0, auditor_getenv was invoked recursively.
  //     in this case, use glibc getenv to break the recursion
  getenv_t getenv_fn =
    ((hpcrun_init_status == STATE_DURING || in_auditor_getenv > 0) ?
     libc_getenv_fn : (getenv_t) outermost_visible_getenv_fn);

  if (verbose) {
    fprintf(stderr, "auditor_getenv(%s): state %d outermost = %p libc = %p ret = %p\n",
      key, hpcrun_init_status, (void *) outermost_visible_getenv_fn, libc_getenv_fn, getenv_fn);
  }

  in_auditor_getenv++; // increment so auditor_getenv can identify recursion
  char *val = getenv_fn(key);
  in_auditor_getenv--; // undo prior increment

  if (verbose) {
    fprintf(stderr, "auditor_getenv(%s)=%s\n", key, val);
  }

  return val;
}



//***************************************************************************
// interface operations
//***************************************************************************

// return non-standard symbol binding when appropriate
bool
audit_symbol_binding
(
  const char *symname,
  uintptr_t *binding
)
{
  bool result = false;

  if (verbose) {
    fprintf(stderr, "symbol_bindings: query symbol=%s (hpcrun_init_status=%d)\n",
      symname, hpcrun_init_status);
  }

  if (strcmp(symname, "hpcrun_connect_to_auditor") == 0) {
    *binding = (uintptr_t) &hpcrun_connect_to_auditor_p;
    result = true;
  }

  else if (strcmp(symname, "getenv") == 0) {
    memoize_first_visible_getenv(*binding);
    *binding = (uintptr_t) &auditor_getenv;
    result = true;
  }

  else {
    // use default binding for any symbol that is not a special case
    result = false;
  }

  if (verbose) {
    fprintf(stderr, "symbol_bindings: symbol=%s, binding=%lx (hpcrun_init_status=%d) result=%s\n",
      symname, *binding, hpcrun_init_status,
      (result ? "'auditor supplied'" : "'program default'"));
  }

  return result;
}


void
libhpcrun_initialization_begin()
{
  hpcrun_init_status = STATE_DURING;

  if (verbose) {
    fprintf(stderr, "libhpcrun_initialization_begin:  hpcrun_init_status --> %d\n", hpcrun_init_status);
  }
}


void
libhpcrun_initialization_end()
{
  hpcrun_init_status = STATE_AFTER;

  if (verbose) {
    fprintf(stderr, "libhpcrun_initialization_end:  hpcrun_init_status --> %d\n", hpcrun_init_status);
  }
}
