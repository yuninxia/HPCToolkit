// SPDX-FileCopyrightText: 2002-2024 Rice University
// SPDX-FileCopyrightText: 2024 Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: BSD-3-Clause

// -*-Mode: C++;-*-

#ifndef LEVEL0_DRIVER_H_
#define LEVEL0_DRIVER_H_

//*****************************************************************************
// level zero includes
//*****************************************************************************

#include <level_zero/ze_api.h>


//*****************************************************************************
// system includes
//*****************************************************************************

#include <iostream>
#include <vector>


//*****************************************************************************
// local includes
//*****************************************************************************

#include "level0-assert.hpp"


//******************************************************************************
// interface operations
//******************************************************************************

void
zeroGetVersion
(
  ze_api_version_t& version
);

std::vector<ze_driver_handle_t>
zeroGetDrivers
(
  void
);

void
zeroCheckDriverVersion
(
  uint32_t requiredMajor,
  uint32_t requiredMinor,
  bool printVersion
);


#endif // LEVEL0_DRIVER_H_