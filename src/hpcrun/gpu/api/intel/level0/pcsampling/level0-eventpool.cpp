// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: BSD-3-Clause

// -*-Mode: C++;-*-

//*****************************************************************************
// system includes
//*****************************************************************************

//*****************************************************************************
// local includes
//*****************************************************************************

#include "level0-eventpool.hpp"
#include "level0-pc-api-receiver.hpp"


//*****************************************************************************
// interface operations
//*****************************************************************************

ze_event_pool_handle_t
level0CreateEventPool
(
  ze_context_handle_t context,
  ze_device_handle_t device,
  uint32_t event_count,
  ze_event_pool_flag_t event_pool_flag,
  const struct hpcrun_foil_appdispatch_level0* dispatch
)
{
  if (context == nullptr) {
    pcsampling::error("Null context handle passed to level0CreateEventPool");
    return nullptr;
  }

  if (device == nullptr) {
    pcsampling::warn("Null device handle passed to level0CreateEventPool");
    return nullptr;
  }

  ze_event_pool_desc_t event_pool_desc = {
    ZE_STRUCTURE_TYPE_EVENT_POOL_DESC, // Structure type
    nullptr,                           // pNext must be null
    event_pool_flag,                   // Event pool flags
    event_count                        // Number of events in the pool
  };

  ze_event_pool_handle_t event_pool = nullptr;

  const uint32_t num_devices = 1; // Single device visibility
  ze_result_t status = pcsampling::callZeEventPoolCreate(
    context,
    &event_pool_desc,
    num_devices,
    &device,
    &event_pool,
    dispatch
  );

  if (status != ZE_RESULT_SUCCESS) {
    pcsampling::error("Failed to create Level Zero event pool: %s", ze_result_to_string(status));
    return nullptr;
  }

  return event_pool;
}
