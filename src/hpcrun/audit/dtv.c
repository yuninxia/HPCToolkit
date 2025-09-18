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

// The definitions for the DTV types below are copied from glibc 2.31 sources.
// This is a workaround until the fix in glibc 2.42 for the DTV management bug
// is broadly disseminated and this code is no longer necessary.

struct dtv_pointer {
  void *val;
  void *to_free;
};

typedef union dtv {
  size_t counter;
  struct dtv_pointer pointer;
} dtv_t;

// type signature for malloc
typedef void* (*malloc_t)(size_t);

//***************************************************************************
// local variables
//***************************************************************************

static dtv_t* glibc_dtv_initial;

//***************************************************************************
// private operations
//***************************************************************************

// Reference:
//   H.J. Liu et al., System V Application Binary Interface AMD64 Architecture
//   Processor Supplement (With LP64 and ILP32 Programming Models), Version 1.0,
//   https://gitlab.com/x86-psABIs/x86-64-ABI.
//
// The %fs segment register is used to implement the thread pointer, implemented
// by type tcbhead_t.
//
// In glibc, the address of the thread control block is stored (inside tcbhead_t)
// at offset 0 relative to %fs. The address of the DTV is at offset 8 from %fs.
// The following code loads the DTV address into register %rax:
//   "movq %fs:8, %rax"

static dtv_t* glibc_dtv_read() {
  void *dtv = NULL;

#if defined(__x86_64__)
  // retrieve the DTV pointer in tcbhead_t at %fs:8 into register %rax. copy
  // it into variable dtv.
  __asm__("mov %%fs:8, %%rax\n"
          "mov %%rax, %0\n"
          : "=r" (dtv)
          :
          : "rax");
#endif

  return dtv;
}

static void glibc_dtv_write(dtv_t* dtv) {
#if defined(__x86_64__)
  // move the value of the argument dtv into register %rax. store the
  // value of %rax info the DTV pointer in tcbhead_t at %fs:8
  __asm__("mov %0, %%rax\n"
          "mov %%rax, %%fs:8\n"
          :
          : "r" (dtv)
          : "rax");
#endif
}

static dtv_t* dtv_reference_to_origin(dtv_t* dtv) {
  // the base pointer for DTV storage is 1 element
  // to the left of the DTV pointer, before a counter
  return dtv - 1;
}

static dtv_t* dtv_origin_to_reference(dtv_t* dtv) {
  // the DTV pointer is 1 element to the right of the
  // base pointer for is storage, after a counter
  return dtv + 1;
}

size_t dtv_elements(dtv_t* dtv) {
  // in glibc, the number of DTV elements is the counter value
  // plus one for the counter itself and one to the right
  size_t elements = dtv[-1].counter + 2;
  return elements;
}

static size_t dtv_size(dtv_t* dtv) {
  size_t elements = dtv_elements(dtv);
  size_t bytes = elements * sizeof(dtv_t);
  return bytes;
}

static dtv_t *dtv_realloc(dtv_t* dtv, malloc_t default_alloc) {
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

void dtv_debug(bool verbose) {
  dtv_t* dtv = glibc_dtv_read();

  if (verbose) {
    fprintf(stderr, "[audit][dtv] dtv_current = %p\n", dtv);
  }
}

void dtv_validate(bool verbose) {
  static bool dtv_finalized = false;

  if (dtv_finalized) return;

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

  malloc_t default_malloc = (malloc_t) dlsym(RTLD_DEFAULT, "malloc");

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

  dtv_finalized = true;
}
