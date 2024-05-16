//==============================================================
// Copyright (C) Intel Corporation
//
// SPDX-License-Identifier: MIT
// =============================================================

#ifndef PTI_UTILS_UTILS_H_
#define PTI_UTILS_UTILS_H_

#include <string>
#include <unistd.h>

#include "pti_assert.h"

namespace utils {

inline std::string GetEnv(const char* name) {
  PTI_ASSERT(name != nullptr);
  const char* value = getenv(name);
  if (value == nullptr) {
    return std::string();
  }
  return std::string(value);
}

} // namespace utils

#endif // PTI_UTILS_UTILS_H_
