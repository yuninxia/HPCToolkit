// SPDX-FileCopyrightText: 2002-2024 Rice University
// SPDX-FileCopyrightText: 2024 Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: BSD-3-Clause

// -*-Mode: C++;-*-

#ifndef LEVEL0_EVENT_HPP
#define LEVEL0_EVENT_HPP

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

ze_event_handle_t
level0CreateEvent
(
  ze_event_pool_handle_t event_pool,                     // [in] handle to the event pool for creating the event
  uint32_t event_index,                                  // [in] index of the event within the pool
  ze_event_scope_flag_t signal_scope,                    // [in] scope of event signal (device, subdevice, or host)
  ze_event_scope_flag_t wait_scope,                      // [in] scope of event wait (device, subdevice, or host)
  const struct hpcrun_foil_appdispatch_level0* dispatch  // [in] level0 dispatch interface
);


#endif // LEVEL0_EVENT_HPP