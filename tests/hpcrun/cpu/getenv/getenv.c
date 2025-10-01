// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: Apache-2.0

// test that calls reach application getenv in and only in the following
// circumstances
// 1. library constructor
// 2. application constructor
// 3. application function
// 4. library function

#define __USE_GNU
#define _GNU_SOURCE

#include <dlfcn.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef char* (*getenv_t)(const char*);

//******************************************************************************
// forward declarations
//******************************************************************************

extern char** environ;

extern void foo();

//******************************************************************************
// local variables
//******************************************************************************

static atomic_int calls = 0; // application getenv hasn't been called yet

//******************************************************************************
// private operations
//******************************************************************************

// while the predicate below returns true, it is useful to keep the abstraction
// because it enables printing to be turned off (if desired in the future)
// without editing at the point of each fprintf
static bool doprint() { return true; }

static void print(const char* where, const char* val) {
  if (doprint()) {
    fprintf(stderr, "%s: calls=%d val=%s\n", where, calls, val);
  }
}

__attribute__((constructor)) static void init() {
  char* val = NULL;
  print("in init before call", val);

  val = getenv("GETENV_TEST_KEY");
  print("in init after call", val);
}

static char* real_getenv(const char* key) {
  static getenv_t libc_getenv_fn = NULL;

  if (libc_getenv_fn == NULL) {
    libc_getenv_fn = (getenv_t)dlsym(RTLD_NEXT, "getenv");
  }

  char* val = libc_getenv_fn(key);

  if (doprint())
    fprintf(stderr, "in real_getenv\n", val);

  return val;
}

//******************************************************************************
// interface operations
//******************************************************************************

// implementation of getenv visible to executable and its shared libraries
char* getenv(const char* key) {

  if (doprint()) {
    fprintf(stderr, "in getenv: key = %s\n", key);
    fflush(0);
  }

  calls++; // note if application getenv is called

  if (strcmp(key, "GETENV_TEST_KEY") == 0)
    return (char*)"GETENV_TEST_VALUE";
  else
    return 0;
}

int main(int argc, char** argv) {
  char* val = NULL;

  print("in main before call", val);

  val = getenv("GETENV_TEST_KEY");

  print("in main after call", val);

  foo();

  return calls != 4; // 1 in init, 1 in main, 1 in libfoo init, 1 in libfoo
}
