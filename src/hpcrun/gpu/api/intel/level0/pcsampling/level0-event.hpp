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

#include "level0-assert.hpp"


//******************************************************************************
// interface operations
//******************************************************************************

ze_event_handle_t
zeroCreateEvent
(
  ze_event_pool_handle_t event_pool,
  uint32_t event_index,
  ze_event_scope_flag_t signal_scope,
  ze_event_scope_flag_t wait_scope
);


#endif // LEVEL0_EVENT_HPP