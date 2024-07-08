//==============================================================
// Copyright (C) Intel Corporation
//
// SPDX-License-Identifier: MIT
// =============================================================

#include "level0-driver.h"
#include "pti_assert.h"

namespace l0_driver {

std::vector<ze_driver_handle_t>
GetDriverList
(
  void
) 
{
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

ze_api_version_t
GetDriverVersion
(
  ze_driver_handle_t driver
) 
{
  PTI_ASSERT(driver != nullptr);
  ze_api_version_t version = ZE_API_VERSION_FORCE_UINT32;
  ze_result_t status = zeDriverGetApiVersion(driver, &version);
  PTI_ASSERT(status == ZE_RESULT_SUCCESS);
  return version;
}

ze_api_version_t
GetVersion
(
  void
) 
{
  auto driver_list = GetDriverList();
  if (driver_list.empty()) {
    return ZE_API_VERSION_FORCE_UINT32;
  }
  return GetDriverVersion(driver_list.front());
}

} // namespace l0_driver