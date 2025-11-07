// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: Apache-2.0

#include <error.h>
#include <pthread.h>

struct parameters {
  unsigned int scale;
};

void* work(void* v_parameters) {
  const struct parameters* parameters = v_parameters;
  volatile double x = 2;
  for (unsigned long long idx = 0,
                          end = ((unsigned long long)1 << 27) * parameters->scale;
       idx < end; ++idx) {
    x = x * 2 + 3;
  }
  return NULL;
}

int main() {
  const struct parameters main_thread_params = {.scale = 1};
  const struct parameters thread_params[] = {
      {.scale = 2},
      {.scale = 5},
  };

  pthread_t threads[sizeof thread_params / sizeof thread_params[0]];
  for (size_t idx = 0; idx < sizeof threads / sizeof threads[0]; ++idx) {
    int err = pthread_create(&threads[idx], NULL, work, (void*)&thread_params[idx]);
    if (err != 0) {
      error(1, err, "pthread_create failed");
    }
  }
  work((void*)&main_thread_params);
  for (size_t idx = 0; idx < sizeof threads / sizeof threads[0]; ++idx) {
    pthread_join(threads[idx], NULL);
  }
  return 0;
}
