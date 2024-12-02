// SPDX-FileCopyrightText: 2002-2024 Rice University
// SPDX-FileCopyrightText: 2024 Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: BSD-3-Clause

// -*-Mode: C++;-*-

#ifndef LEVEL0_EVENTPOOL_HPP
#define LEVEL0_EVENTPOOL_HPP

//*****************************************************************************
// level zero includes
//*****************************************************************************

#include <level_zero/ze_api.h>


//*****************************************************************************
// local includes
//*****************************************************************************

#include "../../../../../foil/level0.h"
#include "level0-assert.hpp"


//******************************************************************************
// interface operations
//******************************************************************************

ze_event_pool_handle_t
zeroCreateEventPool
(
  ze_context_handle_t context,
  ze_device_handle_t device,
  uint32_t event_count,
  ze_event_pool_flag_t event_pool_flag,
  const struct hpcrun_foil_appdispatch_level0* dispatch
);

#endif // LEVEL0_EVENTPOOL_HPP