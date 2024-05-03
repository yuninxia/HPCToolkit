//==============================================================
// Copyright (C) Intel Corporation
//
// SPDX-License-Identifier: MIT
// =============================================================

#ifndef PTI_UTILS_UTILS_H_
#define PTI_UTILS_UTILS_H_

#include <unistd.h>
#include <sys/syscall.h>
#include <stdint.h>

#include <fstream>
#include <string>

#include "pti_assert.h"

namespace utils {

inline void SetEnv(const char* name, const char* value) {
  PTI_ASSERT(name != nullptr);
  PTI_ASSERT(value != nullptr);

  int status = 0;
  status = setenv(name, value, 1);
  PTI_ASSERT(status == 0);
}

inline std::string GetEnv(const char* name) {
  PTI_ASSERT(name != nullptr);
  const char* value = getenv(name);
  if (value == nullptr) {
    return std::string();
  }
  return std::string(value);
}

inline uint32_t GetPid() {
  return getpid();
}

} // namespace utils

#endif // PTI_UTILS_UTILS_H_
