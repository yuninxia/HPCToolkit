// SPDX-FileCopyrightText: 2002-2024 Rice University
// SPDX-FileCopyrightText: 2024 Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: BSD-3-Clause

// -*-Mode: C++;-*-

//*****************************************************************************
// local includes
//*****************************************************************************

#include "level0-eventpool.hpp"


//*****************************************************************************
// private operations
//*****************************************************************************

static ze_event_pool_desc_t
initializeEventPoolDescriptor
(
  uint32_t event_count,
  ze_event_pool_flag_t event_pool_flag
)
{
  ze_event_pool_desc_t desc = {
    ZE_STRUCTURE_TYPE_EVENT_POOL_DESC, // Structure type
    nullptr,                           // pNext must be null
    event_pool_flag,                   // Event pool flags
    event_count                        // Number of events in the pool
  };
  return desc;
}

//*****************************************************************************
// interface operations
//*****************************************************************************

ze_event_pool_handle_t
zeroCreateEventPool
(
  ze_context_handle_t context,
  ze_device_handle_t device,
  uint32_t event_count,
  ze_event_pool_flag_t event_pool_flag,
  const struct hpcrun_foil_appdispatch_level0* dispatch
)
{
  ze_event_pool_desc_t event_pool_desc = initializeEventPoolDescriptor(event_count, event_pool_flag);
  
  ze_event_pool_handle_t event_pool = nullptr;
  
  ze_result_t status = f_zeEventPoolCreate(
    context,
    &event_pool_desc,
    1,          // Number of devices
    &device,
    &event_pool,
    dispatch
  );
  
  level0_check_result(status, __LINE__);
  return event_pool;
}
