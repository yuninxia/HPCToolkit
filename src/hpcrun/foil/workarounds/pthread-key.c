// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: Apache-2.0

/// \file
/// There is a known bug that pthread_keys do not play nice with alternative namespaces
/// in many (all?) versions of Glibc, see
/// https://sourceware.org/bugzilla/show_bug.cgi?id=24776 for all the gritty details. To
/// work around this bug, this file provides the simplest possible implementation for
/// pthread_key functionality, using ELF TLS (i.e. a `thread_local` variable) as the
/// main storage.
///
/// There are a number of notes/caveats to keep in mind here:
///
/// 1. We can't use dlfcn in this implementation because the `dlerror()` result is also
///    stored in a pthread_key (in fact, this is the real impetus for this workaround).
///    While the best approach would be to call `pthread_key_create` in the main link
///    namespace, there isn't a practical way to get that symbol without calling a `dl*`
///    function along the way. So this is the second- or third-best option.
///
/// 2. This implementation ignores pthread_key destructors. There isn't a good way to
///    "know" when a thread actually dies when multiple namespaces are involved. For
///    now, we just ignore the problem and hope it doesn't hurt later.
///
/// 3. `pthread_getspecific` is async-signal-safe (we think) in Glibc, however this
///    implementation is not (https://sourceware.org/glibc/wiki/TLSandSignals). The
///    POSIX standard does not list `pthread_getspecific` as a signal-safe function, so
///    any issues resulting from this change are in fact schrödinbugs.

#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <stdatomic.h>
#include <threads.h>

#define MAX_PTHREAD_KEY 1024
static thread_local void* key_storage[MAX_PTHREAD_KEY] = {0};

static_assert(ATOMIC_LLONG_LOCK_FREE == 2,
              "This atomic needs to be lock-free to avoid potential recursion issues");
static atomic_ullong next_key = 0;

// NOLINTNEXTLINE(bugprone-reserved-identifier): We are overriding the Glibc internal name
__attribute__((visibility("default"))) int __pthread_key_create(pthread_key_t* key,
                                                                void (*destr)(void*)) {
  *key = atomic_fetch_add(&next_key, 1);
  if (*key >= MAX_PTHREAD_KEY) {
    return ENOMEM;
  }
  key_storage[*key] = NULL;
  return 0;
}
__attribute__((visibility("default"))) int pthread_key_create(pthread_key_t* key,
                                                              void (*destr)(void*)) {
  return __pthread_key_create(key, destr);
}

// NOLINTNEXTLINE(bugprone-reserved-identifier): We are overriding the Glibc internal name
__attribute__((visibility("default"))) int __pthread_key_delete(pthread_key_t key) {
  if (key >= MAX_PTHREAD_KEY) {
    return EINVAL;
  }
  key_storage[key] = NULL;
  return 0;
}
__attribute__((visibility("default"))) int pthread_key_delete(pthread_key_t key) {
  return __pthread_key_delete(key);
}

// NOLINTNEXTLINE(bugprone-reserved-identifier): We are overriding the Glibc internal name
__attribute__((visibility("default"))) int __pthread_setspecific(pthread_key_t key,
                                                                 const void* value) {
  if (key >= MAX_PTHREAD_KEY) {
    return EINVAL;
  }
  key_storage[key] = (void*)value;
  return 0;
}
__attribute__((visibility("default"))) int pthread_setspecific(pthread_key_t key,
                                                               const void* value) {
  if (key >= MAX_PTHREAD_KEY) {
    return EINVAL;
  }
  key_storage[key] = (void*)value;
  return 0;
}

// NOLINTNEXTLINE(bugprone-reserved-identifier): We are overriding the Glibc internal name
__attribute__((visibility("default"))) void* __pthread_getspecific(pthread_key_t key) {
  if (key >= MAX_PTHREAD_KEY) {
    return NULL;
  }
  return key_storage[key];
}
__attribute__((visibility("default"))) void* pthread_getspecific(pthread_key_t key) {
  return __pthread_getspecific(key);
}
