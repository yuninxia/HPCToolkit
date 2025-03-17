// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
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

#include "../../../../../foil/level0.h"
#include "level0-assert.hpp"


//******************************************************************************
// interface operations
//******************************************************************************

void
level0GetVersion
(
  ze_api_version_t& version,
  const struct hpcrun_foil_appdispatch_level0* dispatch
);

std::vector<ze_driver_handle_t>
level0GetDrivers
(
  const struct hpcrun_foil_appdispatch_level0* dispatch
);

bool
level0CheckDriverVersion
(
  uint32_t requiredMajor,
  uint32_t requiredMinor,
  bool printVersion,
  const struct hpcrun_foil_appdispatch_level0* dispatch
);


#endif // LEVEL0_DRIVER_H_