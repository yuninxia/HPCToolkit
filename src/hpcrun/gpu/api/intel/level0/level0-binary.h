// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: Apache-2.0

// -*-Mode: C++;-*- // technically C99

#ifndef level0_binary_h
#define level0_binary_h

//*****************************************************************************
// local includes
//*****************************************************************************

#include "../../common/gpu-binary.h"

#include "level0-api.h"



//******************************************************************************
// interface operations
//******************************************************************************

void
level0_binary_process
(
  ze_module_handle_t module,
  const struct hpcrun_foil_appdispatch_level0* dispatch
);

#endif
