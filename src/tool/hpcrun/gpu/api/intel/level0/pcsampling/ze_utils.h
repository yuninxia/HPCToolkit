//==============================================================
// Copyright (C) Intel Corporation
//
// SPDX-License-Identifier: MIT
// =============================================================

#ifndef PTI_UTILS_ZE_UTILS_H_
#define PTI_UTILS_ZE_UTILS_H_

#include <string.h>

#include <chrono>
#include <string>
#include <thread>
#include <vector>

#include <level_zero/ze_api.h>

#include "pti_assert.h"

namespace utils {
namespace ze {

inline std::vector<ze_driver_handle_t> GetDriverList() {
  ze_result_t status = ZE_RESULT_SUCCESS;

  uint32_t driver_count = 0;
  status = zeDriverGet(&driver_count, nullptr);
  PTI_ASSERT(status == ZE_RESULT_SUCCESS);

  if (driver_count == 0) {
    return std::vector<ze_driver_handle_t>();
  }

  std::vector<ze_driver_handle_t> driver_list(driver_count);
  status = zeDriverGet(&driver_count, driver_list.data());
  PTI_ASSERT(status == ZE_RESULT_SUCCESS);

  return driver_list;
}

inline ze_api_version_t GetDriverVersion(ze_driver_handle_t driver) {
  PTI_ASSERT(driver != nullptr);

  ze_api_version_t version = ZE_API_VERSION_FORCE_UINT32;
  ze_result_t status = zeDriverGetApiVersion(driver, &version);
  PTI_ASSERT(status == ZE_RESULT_SUCCESS);

  return version;
}

inline ze_api_version_t GetVersion() {
  auto driver_list = GetDriverList();
  if (driver_list.empty()) {
    return ZE_API_VERSION_FORCE_UINT32;
  }
  return GetDriverVersion(driver_list.front());
}

} // namespace ze
} // namespace utils

#endif // PTI_UTILS_ZE_UTILS_H_
