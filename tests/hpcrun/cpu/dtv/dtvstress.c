// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: Apache-2.0

#define _GNU_SOURCE
#include <dlfcn.h>
#include <libgen.h>
#include <limits.h> // For PATH_MAX
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define DEBUG 1

#define SUCCESS 0
#define FAILURE -1

typedef void (*set_t)(int);

/// @brief Signal handler for segmentation faults (SIGABRT).
/// @param signum The signal number that triggered the handler (should be SIGABRT).
void abort_handler(int signum) {
  printf("Caught SIGABRT!\n");
  exit(FAILURE);
}

char* exec_directory() {
  static char exe_path[PATH_MAX];

  ssize_t len = readlink("/proc/self/exe", exe_path, sizeof(exe_path) - 1);

  if (len == -1) {
    perror("readlink");
    exit(FAILURE);
  }

  exe_path[len] = '\0'; // Null-terminate the string

  return dirname(exe_path);
}

void openlib(char* dir, char* num) {
  char name[PATH_MAX];

  snprintf(name, PATH_MAX, "%s/libdlopentls%s.so", dir, num);
#if DEBUG
  printf("opening %s\n", name);
#endif

  void* result = dlopen(name, RTLD_NOW);
  if (!result) {
    fprintf(stderr, "Error opening library %s: %s\n", name, dlerror());
    exit(FAILURE);
  }

  snprintf(name, PATH_MAX, "dlopen_set_%s", num);
  set_t set = (set_t)dlsym(result, name);
  if (!set) {
    fprintf(stderr, "Error looking up symbol %s\n", name);
    exit(FAILURE);
  }
  set(4);
}

int main(int argc, char* argv[]) {
  // Set up a signal handler to catch a SEGV so that failure
  // is reported with a bad return code rather than a core dump.
  if (signal(SIGABRT, abort_handler) == SIG_ERR) {
    perror("Failed to set up signal handler");
    return FAILURE;
  }

  // Get the directory containing the program and its libraries
  char* dir = exec_directory();

  for (int i = 1; i < 16; i++) {
    char digits[3];
    sprintf(digits, "%02d", i);
    // Open the shared library in 'dir' identified by digits.
    openlib(dir, digits);
  }
  printf("Success\n");

  return SUCCESS;
}
