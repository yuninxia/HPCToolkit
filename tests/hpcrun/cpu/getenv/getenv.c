// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: Apache-2.0

// test that calls reach application getenv in if and only if in the following circumstances
// (1) library constructor
// (2) application constructor
// (3) application function
// (4) library function

#define __USE_GNU
#define _GNU_SOURCE

#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>

typedef char * (*getenv_t)(const char *);


//******************************************************************************
// forward declarations
//******************************************************************************

extern char **environ;

extern void foo();



//******************************************************************************
// local variables
//******************************************************************************

static atomic_int calls = 0; // application getenv hasn't been called yet



//******************************************************************************
// private operations
//******************************************************************************

static int doprint() { 
  return strstr(environ[0], "getenv") != 0;
}


static void print(const char *where, const char *val)
{
  if (doprint()) { 
    fprintf(stderr, "%s: calls=%d val=%s\n", where, calls, val);
  }
}


__attribute__((constructor))
static void init()
{
  char *val = 0;
  print("in init before call", val);

  val = getenv("FOO");
  print("in init after call", val);
}


static char *real_getenv(const char *key)
{
  static getenv_t libc_getenv_fn = 0;

  if (libc_getenv_fn == 0) {
    libc_getenv_fn = (getenv_t) dlsym(RTLD_NEXT, "getenv");
  }

  char *val = libc_getenv_fn(key);

  if (doprint()) fprintf(stderr, "in real_getenv\n", val);
  
  return val;
}



//******************************************************************************
// interface operations
//******************************************************************************

// implementation of getenv visible to executable and its shared libraries
char *getenv(const char *key) {

  if (doprint()) {
   fprintf(stderr, "in getenv: key = %s\n", key);
   fflush(0);
  }

  calls++; // note if application getenv is called 

  if (strcmp(key, "FOO") == 0) return (char *)"BAR";
  else return 0;
}


int main(int argc, char **argv) {
  char *val = 0;

  print("in main before call", val);

  val = getenv("FOO");

  print("in main after call", val);

  foo();

  return calls != 4; // 1 in init, 1 in main, 1 in libfoo init, 1 in libfoo
}
