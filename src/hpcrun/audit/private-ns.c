// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: BSD-3-Clause

// -*-Mode: C++;-*-

#define _GNU_SOURCE

#include "binding.h"

#include <assert.h>
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>

__attribute__((visibility("default")))
void hpcrun_bind_v(const char* libname, va_list bindings) {
  // Before anything else, try to load the library.
  void* handle = dlopen(libname, RTLD_NOW | RTLD_LOCAL);
  if (handle == NULL) {
    fprintf(stderr, "ERROR: Unable to bind to '%s': failed to load: %s\n", libname, dlerror());
    abort();
  }

  // Bind each requested symbol in turn
  dlerror();  // Clear any errors
  for (const char* sym = va_arg(bindings, const char*); sym != NULL;
       sym = va_arg(bindings, const char*)) {
    void** dst = va_arg(bindings, void**);
    *dst = dlsym(handle, sym);
    const char* error = dlerror();
    if (*dst == NULL && error != NULL) {
      fprintf(stderr, "ERROR: Unable to bind to '%s': %s\n", libname, error);
      abort();
    }
  }
}
