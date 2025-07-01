// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: Apache-2.0

#define _GNU_SOURCE
#include <dlfcn.h>
#include <error.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static pthread_key_t key;
static uint8_t canary[256];
#define CANARY_VALUE 0xdd
static void do_setup() {
  memset(canary, CANARY_VALUE, sizeof canary);

  int ret = pthread_key_create(&key, NULL);
  if (ret != 0) {
    error(1, ret, "pthread_key_create failed");
  }
  ret = pthread_setspecific(key, canary);
  if (ret != 0) {
    error(1, ret, "pthread_setspecific failed");
  }

  // In order for this to be a valid test, we need pthread_key 0. Otherwise we
  // might not overlap correctly with the private namespace.
  if (key != 0) {
    fprintf(stderr,
            "pthread_key_create was called too late, wanted key 0 but got key "
            "%d.\n  This is likely because the hpcrun init sequence has "
            "changed, probably for the better.\n  Consider intercepting "
            "another function with a call to setup() or malloc().\nThe test will now "
            "continue but might pass unexpectedly.\n",
            key);
  }
}
static void setup() {
  static pthread_once_t once = PTHREAD_ONCE_INIT;
  pthread_once(&once, do_setup);
}

int main() {
  setup();

  extern void helper();
  helper();

  // Test that our canary has not be overwritten during the startup sequence
  const uint8_t* tls = pthread_getspecific(key);
  if (tls != canary) {
    error(1, 0, "pthread_getspecific did not return canary %p, got %p", canary, tls);
  }
  for (size_t idx = 0; idx < sizeof canary; ++idx) {
    if (tls[idx] != CANARY_VALUE) {
      error(1, 0, "canary was overwritten: tls[%zu] = %hhu, expected %d", idx, tls[idx],
            CANARY_VALUE);
    }
  }

  return 0;
}

// To nab pthread_key 0, we need to run very early in the process. The allocator
// is a good injection point.
extern void* __libc_malloc(size_t size);
void* malloc(size_t size) {
  setup();
  return __libc_malloc(size);
}
extern void* __libc_calloc(size_t nb, size_t size);
void* calloc(size_t nb, size_t size) {
  setup();
  return __libc_calloc(nb, size);
}
extern void* __libc_realloc(void* ptr, size_t size);
void* realloc(void* ptr, size_t size) {
  setup();
  return __libc_realloc(ptr, size);
}
