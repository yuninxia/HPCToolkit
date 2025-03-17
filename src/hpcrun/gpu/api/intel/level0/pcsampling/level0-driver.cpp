// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
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
  if (driver == nullptr) {
    std::cerr << "[ERROR] Null driver handle passed to fetchDriverVersion" << std::endl;
    // Set version to a known invalid value to indicate error condition
    // ZE_API_VERSION_FORCE_UINT32 is defined by Level Zero API as a special constant
    // that represents an invalid or uninitialized version
    version = ZE_API_VERSION_FORCE_UINT32;
    return;
  }

  ze_result_t status = f_zeDriverGetApiVersion(driver, &version, dispatch);
  if (status != ZE_RESULT_SUCCESS) {
    std::cerr << "[ERROR] Failed to get driver API version: " << ze_result_to_string(status) << std::endl;
    // Set version to a known invalid value to indicate error condition
    version = ZE_API_VERSION_FORCE_UINT32;
  }
}

static bool
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
    std::cerr << "[ERROR] Level Zero API version " << major << "." << minor << " is lower than required " << requiredMajor << "." << requiredMinor << std::endl;
    return false;
  }
  
  return true;
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
  auto drivers = level0GetDrivers(dispatch);
  if (drivers.empty()) {
    std::cerr << "[ERROR] No Level Zero drivers available" << std::endl;
    // Set version to a known invalid value to indicate error condition
    version = ZE_API_VERSION_FORCE_UINT32;
    return;
  }
  
  fetchDriverVersion(drivers[0], version, dispatch);
}

std::vector<ze_driver_handle_t>
level0GetDrivers
(
  const struct hpcrun_foil_appdispatch_level0* dispatch
)
{
  return fetchDriverHandles(dispatch);
}

bool
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
  return validateAndPrintDriverVersion(version, requiredMajor, requiredMinor, printVersion);
}
