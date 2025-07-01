// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: Apache-2.0

/// \file
/// libunwind calls dl_iterate_phdr to acquire information about the load map, but
/// dl_iterate_phdr is not async-signal-safe and will deadlock when called from a signal
/// handler.
///
/// The function and pointer below allow reimplementing the function with a custom
/// version. The pointer must be filled with a suitable override before any calls to the
/// function occur.

#define _GNU_SOURCE
#include <assert.h>
#include <link.h>
#include <stdbool.h>
#include <stdlib.h>

typedef int (*pfn_iterate_phdr_t)(int (*callback)(struct dl_phdr_info*, size_t, void*),
                                  void* data);
__attribute__((visibility("default"))) pfn_iterate_phdr_t hpcrun_iterate_phdr = NULL;
__attribute__((visibility("default"))) int
dl_iterate_phdr(int (*callback)(struct dl_phdr_info*, size_t, void*), void* data) {
  if (hpcrun_iterate_phdr == NULL) {
    assert(
        false &&
        "dl_iterate_phdr called but hpcrun_iterate_phdr override pointer not yet set!");
    abort();
  }
  return hpcrun_iterate_phdr(callback, data);
}
