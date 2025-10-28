// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: Apache-2.0

// -*-Mode: C++;-*-

// This file provides a wrapper around libiberty's cplus_demangle() to
// provide a uniform interface for the options that we want for
// hpcstruct and hpcprof.  All cases wanting to do demangling should
// use this file.
//
// Libiberty cplus_demangle() does many malloc()s, but does appear to
// be reentrant and thread-safe.  But not signal safe.

//***************************************************************************

#include <string.h>

#include "gnu_demangle.h"
#include "hpctoolkit_demangle.h"

#define DEMANGLE_FLAGS  (DMGL_PARAMS | DMGL_ANSI | DMGL_VERBOSE | DMGL_RET_DROP)

// Returns: malloc()ed string for the demangled name, or else NULL if
// 'name' is not a mangled name.
//
// Note: the caller is responsible for calling free() on the result.
//
char *
hpctoolkit_demangle(const char * name)
{
  if (name == NULL) {
    return NULL;
  }

  if (strncmp(name, "_Z", 2) != 0) {
    return NULL;
  }

  // cplus_demangle() by itself does not need a lock.  But if we add
  // cplus_demangle_set_style() to set global state, then use
  // pthread_once().

  char *demangled = cplus_demangle(name, DEMANGLE_FLAGS);

  return demangled;
}
