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
#include <vector>

#include "pti_assert.h"

#define PTI_EXPORT __attribute__ ((visibility ("default")))

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

#define MAX_STR_SIZE 1024

#define BYTES_IN_MBYTES (1024 * 1024)

#define NSEC_IN_USEC 1000
#define MSEC_IN_SEC  1000
#define NSEC_IN_MSEC 1000000
#define NSEC_IN_SEC  1000000000

namespace utils {

inline std::string GetFilePath(const std::string& filename) {
  PTI_ASSERT(!filename.empty());

  size_t pos = filename.find_last_of("/\\");
  if (pos == std::string::npos) {
    return "";
  }

  return filename.substr(0, pos + 1);
}

inline std::string GetExecutablePath() {
  char buffer[MAX_STR_SIZE] = { 0 };
  ssize_t status = readlink("/proc/self/exe", buffer, MAX_STR_SIZE);
  PTI_ASSERT(status > 0);
  return GetFilePath(buffer);
}

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

inline uint32_t GetTid() {
#ifdef SYS_gettid
  return (uint32_t)syscall(SYS_gettid);
#else
  #error "SYS_gettid is unavailable on this system"
#endif
}

} // namespace utils

#endif // PTI_UTILS_UTILS_H_
