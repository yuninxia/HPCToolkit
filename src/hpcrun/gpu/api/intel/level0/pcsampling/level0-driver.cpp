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

static void
zeroGetDriverList
(
  std::vector<ze_driver_handle_t>& driver_list,
  const struct hpcrun_foil_appdispatch_level0* dispatch
)
{
  uint32_t driver_count = 0;
  ze_result_t status = f_zeDriverGet(&driver_count, nullptr, dispatch);
  level0_check_result(status, __LINE__);

  driver_list.clear();
  if (driver_count == 0) {
    return;
  }

  driver_list.resize(driver_count);
  status = f_zeDriverGet(&driver_count, driver_list.data(), dispatch);
  level0_check_result(status, __LINE__);
}

static void
zeroGetDriverVersion
(
  ze_driver_handle_t driver,
  ze_api_version_t& version,
  const struct hpcrun_foil_appdispatch_level0* dispatch
)
{
  assert(driver != nullptr);
  ze_result_t status = f_zeDriverGetApiVersion(driver, &version, dispatch);
  level0_check_result(status, __LINE__);
}


//******************************************************************************
// interface operations
//******************************************************************************

void
zeroGetVersion
(
  ze_api_version_t& version,
  const struct hpcrun_foil_appdispatch_level0* dispatch
)
{
  std::vector<ze_driver_handle_t> driver_list;
  zeroGetDriverList(driver_list, dispatch);
  if (driver_list.empty()) {
    version = ZE_API_VERSION_FORCE_UINT32;
  } else {
    zeroGetDriverVersion(driver_list.front(), version, dispatch);
  }
}

std::vector<ze_driver_handle_t>
zeroGetDrivers
(
  const struct hpcrun_foil_appdispatch_level0* dispatch
)
{
  uint32_t num_drivers = 0;
  ze_result_t status = f_zeDriverGet(&num_drivers, nullptr, dispatch);
  level0_check_result(status, __LINE__);
  
  if (num_drivers == 0) {
      return {};
  }
  
  std::vector<ze_driver_handle_t> drivers(num_drivers);
  status = f_zeDriverGet(&num_drivers, drivers.data(), dispatch);
  level0_check_result(status, __LINE__);
  
  return drivers;
}

void
zeroCheckDriverVersion
(
  uint32_t requiredMajor,
  uint32_t requiredMinor,
  bool printVersion,
  const struct hpcrun_foil_appdispatch_level0* dispatch
)
{
  ze_api_version_t version;
  zeroGetVersion(version, dispatch);

  uint32_t major = ZE_MAJOR_VERSION(version);
  uint32_t minor = ZE_MINOR_VERSION(version);

  if (printVersion) {
    std::cout << "Level Zero API version: " << major << "." << minor << std::endl;
  }

  assert((major >= requiredMajor && minor >= requiredMinor) && 
          "Level Zero API version is lower than required");
}
