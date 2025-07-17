// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: Apache-2.0

#include <stdlib.h>
#include <string.h>

int main() {
  void* x = malloc(1024);
  memset(x, 42, 1024);

  // TODO: memalign (deprecated) / posix_memalign (POSIX) / aligned_alloc
  // TODO: valloc (deprecated)
  // TODO: calloc
  // TODO: free
  // TODO: realloc

  return 0;
}
