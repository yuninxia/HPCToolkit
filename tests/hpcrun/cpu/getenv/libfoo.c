// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: Apache-2.0

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

extern char** environ;

extern char* getenv(const char* key);

static bool doprint() { return true; }

void foo() {
  char* val = 0;

  if (doprint())
    fprintf(stderr, "in foo before call: val=%s\n", val);

  val = getenv("GETENV_TEST_KEY");

  if (doprint())
    fprintf(stderr, "in foo after call: val=%s\n", val);
}

__attribute__((constructor)) void libfoo_init() {
  char* val = 0;

  if (doprint())
    fprintf(stderr, "in libfoo_init before call: val=%s\n", val);

  val = getenv("GETENV_TEST_KEY");

  if (doprint())
    fprintf(stderr, "in libfoo_init after call: val=%s\n", val);
}
