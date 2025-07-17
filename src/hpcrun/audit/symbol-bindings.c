// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: BSD-3-Clause

// -*-Mode: C++;-*- // technically C99

//***************************************************************************
// system includes
//***************************************************************************

#define _GNU_SOURCE
#include <dlfcn.h>  // for dlopen, dlsym, and RTLD_LAZY 

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
  SYMBOL_BINDER_DEFAULT = 0,   // ignore special auditor bindings of symbol
  SYMBOL_BINDER_AUDITOR = 1    // consider special auditor bindings of symbol
} symbol_binder_t;

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

// variable used to control consideration of auditor symbol bindings. this is
// thread local to avoid races between threads
static __thread symbol_binder_t symbol_binder = SYMBOL_BINDER_AUDITOR;

static bool verbose = false;



//***************************************************************************
// forward declarations
//***************************************************************************

extern const auditor_exports_t* hpcrun_connect_to_auditor_p();



//***************************************************************************
// private operations
//***************************************************************************

// return binding of getenv in the context of the executable. if the
// executable defines getenv, return that a pointer to that. if it does not,
// return  a pointer to glibc's getenv
static getenv_t
resolve_program_getenv
(
  void
)
{
  // FIX ME dlmopen LM_ID_BASE? use *some* entry from  link map saved by la_objopen?
  void *handle = dlopen(NULL, RTLD_LAZY);

  if (handle == 0) {
    fprintf(stderr, "hpcrun: FATAL error: unable to dlopen the executable\n");
    exit(-1);
  }

  // ignore auditor special bindings
  symbol_binder = SYMBOL_BINDER_DEFAULT;

  getenv_t pgm_getenv = dlsym(handle, "getenv");

  // reset state to consider auditor special bindings
  symbol_binder = SYMBOL_BINDER_AUDITOR;

  dlclose(handle);

  // result = a binding of getenv in (1) the executable or (2) glibc getenv
  getenv_t result = pgm_getenv ? pgm_getenv : &getenv;

  if (verbose) {
    fprintf(stderr, "resolve_program_getenv: result = %p, pgm getenv = %p, glibc getenv = %p\n",
      result, pgm_getenv, &getenv);
  }

  return result;
}


static char *
auditor_getenv
(
  const char *key
)
{
  static __thread int in_auditor_getenv = 0; // > 0 means recursive invocation

  static getenv_t libc_getenv_fn = &getenv;
  static getenv_t pgm_getenv_fn = 0;

  if (pgm_getenv_fn == 0) {
    pgm_getenv_fn = resolve_program_getenv();
  }

  // (1) during libhpcrun.so initialization, use glibc getenv
  // (2) if in_auditor_getenv > 0, auditor_getenv was invoked recursively.
  //     in this case, use glibc getenv to break the recursion
  getenv_t getenv_fn =
    ((hpcrun_init_status == STATE_DURING || in_auditor_getenv > 0) ?
     libc_getenv_fn : pgm_getenv_fn);

  if (verbose) {
    fprintf(stderr, "auditor_getenv(%s): state %d pgm = %p get = %p ret = %p\n",
      key, hpcrun_init_status, pgm_getenv_fn, libc_getenv_fn, getenv_fn);
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
    fprintf(stderr, "symbol_bindings: query symbol=%s, who=%d (hpcrun_init_status=%d)\n",
      symname, symbol_binder, hpcrun_init_status);
  }

  if (symbol_binder == SYMBOL_BINDER_DEFAULT) {
    // 'result = false' means use default symbol binding
    result = false;
  }

  else if (strcmp(symname, "hpcrun_connect_to_auditor") == 0) {
    *binding = (uintptr_t) &hpcrun_connect_to_auditor_p;
    result = true;
  }

  else if (strcmp(symname, "getenv") == 0) {
    *binding = (uintptr_t) &auditor_getenv;
    result = true;
  }

  else {
    // use default binding for any symbol that is not a special case
    result = false;
  }

  if (verbose) {
    fprintf(stderr, "symbol_bindings: symbol=%s, who=%d binding=%lx (hpcrun_init_status=%d) result=%s\n",
      symname, symbol_binder, *binding, hpcrun_init_status,
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
