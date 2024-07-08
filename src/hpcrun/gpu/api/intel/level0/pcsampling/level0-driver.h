//==============================================================
// Copyright (C) Intel Corporation
//
// SPDX-License-Identifier: MIT
// =============================================================

#ifndef LEVEL0_DRIVER_H_
#define LEVEL0_DRIVER_H_

#include <level_zero/ze_api.h>
#include <vector>

namespace l0_driver {

std::vector<ze_driver_handle_t>
GetDriverList
(
  void
);

ze_api_version_t
GetDriverVersion
(
  ze_driver_handle_t driver
);

ze_api_version_t
GetVersion
(
  void
);

} // namespace l0_driver

#endif // LEVEL0_DRIVER_H_