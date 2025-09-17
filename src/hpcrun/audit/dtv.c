// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: Apache-2.0

// -*-Mode: C++;-*- // technically C99

#define _GNU_SOURCE

//***************************************************************************
// local includes
//***************************************************************************

#include "dtv.h"

//***************************************************************************
// global includes
//***************************************************************************

#include <link.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

//***************************************************************************
// type declarations
//***************************************************************************

// The definition for the dtv_t structure below mirrors that in the glibc
// sources. This is a workaround until the fix in glibc 2.42 for the DTV
// management bug is broadly disseminated.

typedef struct dtv_slot_t {
  void* dont_care1;
  void* dont_care2;
} dtv_slot_t;

typedef union dtv {
  size_t counter;
  dtv_slot_t dont_care;
} dtv_t;

typedef void* (*alloc_t)(size_t);

//***************************************************************************
// local variables
//***************************************************************************

static dtv_t* glibc_dtv_initial;

//***************************************************************************
// private operations
//***************************************************************************

static dtv_t* glibc_dtv_read() {
  void *dtv = NULL;

#if defined(__x86_64__)
  __asm__("mov %%fs:0x8, %%rax\n"
          "mov %%rax, %0\n"
          : "=r" (dtv)
          :
          : "rax");
#endif

  return dtv;
}

static void glibc_dtv_write(dtv_t* dtv) {
#if defined(__x86_64__)
  __asm__("mov %0, %%rax\n"
          "mov %%rax, %%fs:0x8\n"
          :
          : "r" (dtv)
          : "rax");
#endif
}

static size_t dtv_size(dtv_t* dtv) {
  size_t slots = dtv[-1].counter + 2;
  size_t bytes = slots * sizeof(dtv_t);
  return bytes;
}

static dtv_t* dtv_reference_to_origin(dtv_t* dtv) {
  return dtv - 1;
}

static dtv_t* dtv_origin_to_reference(dtv_t* dtv) {
  return dtv + 1;
}

static dtv_t *dtv_realloc(dtv_t* dtv, alloc_t default_alloc) {
  size_t size = dtv_size(dtv);

  // allocate space in the default namespace for a copy of dtv
  dtv_t* dtv_new = (dtv_t*) default_alloc(size);

  // copy dtv into the heap, starting from its origin
  memcpy(dtv_new, dtv_reference_to_origin(dtv), size);

  // return a pointer to the reference location of the dtv
  return dtv_origin_to_reference(dtv_new);
}

//***************************************************************************
// public operations
//***************************************************************************

void dtv_capture_initial(bool verbose) {
  glibc_dtv_initial = glibc_dtv_read();

  if (verbose) {
    fprintf(stderr, "[audit][dtv] glibc_dtv_initial = %p\n",
      glibc_dtv_initial);
  }
}

void dtv_finalize(bool verbose) {
  static bool dtv_finalized = false;

  if (dtv_finalized) return;

  dtv_finalized = true;

  dtv_t* dtv_current = glibc_dtv_read();

  if (dtv_current == 0) {
    if (verbose) {
      fprintf(stderr, "[audit][dtv] glibc DTV reallocation bug unhandled for "
        "this architecture.\n");
    }
    return;
  }

  if (dtv_current == glibc_dtv_initial) {
    if (verbose) {
      fprintf(stderr, "[audit][dtv] dtv_current == glibc_dtv_initial at "
        "finalize: no reallocation necessary\n");
    }
    return;
  }

  // DTV reallocation is necessary. glibc has already reallocated the DTV
  // so that it is no longer glibc_dtv_initial. In buggy versions of glibc
  // (1) this new copy of the DTV will also have been allocated using
  //     rtld_malloc, and
  // (2) glibc will be unaware that it is unsafe to update its allocation
  //     with realloc.

  alloc_t default_malloc = (alloc_t) dlsym(RTLD_DEFAULT, "malloc");

  if (default_malloc == 0) {
    fprintf(stderr, "[audit][dtv] malloc not found in default namespace.\n");
    return;
  }

  dtv_t* dtv_heap = dtv_realloc(dtv_current, default_malloc);

  if (verbose) {
    fprintf(stderr, "[audit][dtv] reallocating: initial %p, current %p, new %p\n",
      glibc_dtv_initial, dtv_current, dtv_heap);
  }

  glibc_dtv_write(dtv_heap);
}
