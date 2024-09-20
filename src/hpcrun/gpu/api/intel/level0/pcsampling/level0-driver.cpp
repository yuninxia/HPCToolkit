// SPDX-FileCopyrightText: 2002-2024 Rice University
// SPDX-FileCopyrightText: 2024 Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: BSD-3-Clause

// -*-Mode: C++;-*-

//*****************************************************************************
// local includes
//*****************************************************************************

#include "level0-driver.h"


//******************************************************************************
// private operations
//******************************************************************************

void
zeroGetDriverList
(
  std::vector<ze_driver_handle_t>& driver_list
)
{
  uint32_t driver_count = 0;
  ze_result_t status = zeDriverGet(&driver_count, nullptr);
  PTI_ASSERT(status == ZE_RESULT_SUCCESS);

  driver_list.clear();
  if (driver_count == 0) {
    return;
  }

  driver_list.resize(driver_count);
  status = zeDriverGet(&driver_count, driver_list.data());
  PTI_ASSERT(status == ZE_RESULT_SUCCESS);
}

void
zeroGetDriverVersion
(
  ze_driver_handle_t driver,
  ze_api_version_t& version
)
{
  PTI_ASSERT(driver != nullptr);
  ze_result_t status = zeDriverGetApiVersion(driver, &version);
  PTI_ASSERT(status == ZE_RESULT_SUCCESS);
}


//******************************************************************************
// interface operations
//******************************************************************************

void
zeroGetVersion
(
  ze_api_version_t& version
)
{
  std::vector<ze_driver_handle_t> driver_list;
  zeroGetDriverList(driver_list);
  if (driver_list.empty()) {
    version = ZE_API_VERSION_FORCE_UINT32;
  } else {
    zeroGetDriverVersion(driver_list.front(), version);
  }
}
