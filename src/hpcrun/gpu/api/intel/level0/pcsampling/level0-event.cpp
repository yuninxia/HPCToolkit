// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: BSD-3-Clause

// -*-Mode: C++;-*-

//*****************************************************************************
// system includes
//*****************************************************************************

#include <iostream>

//*****************************************************************************
// local includes
//*****************************************************************************

#include "level0-event.hpp"
#include "pcsampling-api-receiver.hpp"


//******************************************************************************
// interface operations
//******************************************************************************

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
  if (event_pool == nullptr) {
    std::cerr << "[ERROR] Null event pool handle passed to level0CreateEvent" << std::endl;
    return nullptr;
  }

  ze_event_desc_t event_desc = {
    ZE_STRUCTURE_TYPE_EVENT_DESC, // Structure type
    nullptr,                      // pNext must be null
    event_index,                  // Event index within the pool
    signal_scope,                 // Signal scope flags
    wait_scope                    // Wait scope flags
  };

  ze_event_handle_t event = nullptr;
  ze_result_t status = pcsampling::callZeEventCreate(event_pool, &event_desc, &event, dispatch);
  if (status != ZE_RESULT_SUCCESS) {
    std::cerr << "[ERROR] Failed to create Level Zero event at line " << __LINE__
              << ": " << ze_result_to_string(status) << std::endl;
    return nullptr;
  }

  return event;
}
