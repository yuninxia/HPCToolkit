// SPDX-FileCopyrightText: 2002-2024 Rice University
// SPDX-FileCopyrightText: 2024 Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: BSD-3-Clause

// -*-Mode: C++;-*-

//*****************************************************************************
// local includes
//*****************************************************************************

#include "level0-event.hpp"


//******************************************************************************
// private operations
//******************************************************************************

static ze_event_desc_t
initializeEventDescriptor
(
  uint32_t event_index,
  ze_event_scope_flag_t signal_scope,
  ze_event_scope_flag_t wait_scope
)
{
  ze_event_desc_t event_desc = {
    ZE_STRUCTURE_TYPE_EVENT_DESC, // Structure type
    nullptr,                      // pNext must be null
    event_index,                  // Event index within the pool
    signal_scope,                 // Signal scope flags
    wait_scope                    // Wait scope flags
  };
  return event_desc;
}

//*****************************************************************************
// Interface Operations
//*****************************************************************************

ze_event_handle_t
level0CreateEvent
(
  ze_event_pool_handle_t event_pool,
  uint32_t event_index,
  ze_event_scope_flag_t signal_scope,
  ze_event_scope_flag_t wait_scope,
  const struct hpcrun_foil_appdispatch_level0* dispatch
)
{
  ze_event_desc_t event_desc = initializeEventDescriptor(event_index, signal_scope, wait_scope);
  
  ze_event_handle_t event = nullptr;
  ze_result_t status = f_zeEventCreate(event_pool, &event_desc, &event, dispatch);
  level0_check_result(status, __LINE__);
  return event;
}
