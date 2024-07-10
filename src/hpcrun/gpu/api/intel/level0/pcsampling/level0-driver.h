//==============================================================
// Copyright (C) Intel Corporation
//
// SPDX-License-Identifier: MIT
// =============================================================

#ifndef LEVEL0_DRIVER_H_
#define LEVEL0_DRIVER_H_

#include <level_zero/ze_api.h>

#include <vector>

#include "pti_assert.h"

void
zeroGetDriverList
(
  std::vector<ze_driver_handle_t>& driver_list
);

void
zeroGetDriverVersion
(
  ze_driver_handle_t driver,
  ze_api_version_t& version
);

void
zeroGetVersion
(
  ze_api_version_t& version
);

#endif // LEVEL0_DRIVER_H_