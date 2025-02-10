// SPDX-FileCopyrightText: 2002-2024 Rice University
// SPDX-FileCopyrightText: 2024 Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: BSD-3-Clause

// -*-Mode: C++;-*-

//*****************************************************************************
// local includes
//*****************************************************************************

#include "level0-driver.hpp"


//******************************************************************************
// private operations
//******************************************************************************

static uint32_t
fetchDriverCount
(
  const struct hpcrun_foil_appdispatch_level0* dispatch
)
{
  uint32_t driver_count = 0;
  ze_result_t status = f_zeDriverGet(&driver_count, nullptr, dispatch);
  level0_check_result(status, __LINE__);
  return driver_count;
}

static std::vector<ze_driver_handle_t>
fetchDriverHandles
(
  const struct hpcrun_foil_appdispatch_level0* dispatch
)
{
  uint32_t driver_count = fetchDriverCount(dispatch);
  if (driver_count == 0) {
    return {};
  }
  
  std::vector<ze_driver_handle_t> drivers(driver_count);
  ze_result_t status = f_zeDriverGet(&driver_count, drivers.data(), dispatch);
  level0_check_result(status, __LINE__);
  
  return drivers;
}

static void
fetchDriverVersion
(
  ze_driver_handle_t driver,
  ze_api_version_t& version,
  const struct hpcrun_foil_appdispatch_level0* dispatch
)
{
  assert(driver != nullptr && "Invalid driver handle");
  ze_result_t status = f_zeDriverGetApiVersion(driver, &version, dispatch);
  level0_check_result(status, __LINE__);
}

static void
validateAndPrintDriverVersion
(
  const ze_api_version_t version,
  uint32_t requiredMajor,
  uint32_t requiredMinor,
  bool printVersion
)
{
  uint32_t major = ZE_MAJOR_VERSION(version);
  uint32_t minor = ZE_MINOR_VERSION(version);

  if (printVersion) {
    std::cout << "Level Zero API version: " << major << "." << minor << std::endl;
  }

  // Verify that the driver version is acceptable.
  if ((major < requiredMajor) || (major == requiredMajor && minor < requiredMinor)) {
    assert(false && "Level Zero API version is lower than required");
  }
}

//*****************************************************************************
// Interface Operations
//*****************************************************************************

void
level0GetVersion
(
  ze_api_version_t& version,
  const struct hpcrun_foil_appdispatch_level0* dispatch
)
{
  std::vector<ze_driver_handle_t> driver_list = fetchDriverHandles(dispatch);
  if (driver_list.empty()) {
    version = ZE_API_VERSION_FORCE_UINT32;
  } else {
    fetchDriverVersion(driver_list.front(), version, dispatch);
  }
}

std::vector<ze_driver_handle_t>
level0GetDrivers
(
  const struct hpcrun_foil_appdispatch_level0* dispatch
)
{
  return fetchDriverHandles(dispatch);
}

void
level0CheckDriverVersion
(
  uint32_t requiredMajor,
  uint32_t requiredMinor,
  bool printVersion,
  const struct hpcrun_foil_appdispatch_level0* dispatch
)
{
  ze_api_version_t version;
  level0GetVersion(version, dispatch);
  validateAndPrintDriverVersion(version, requiredMajor, requiredMinor, printVersion);
}
