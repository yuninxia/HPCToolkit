// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: Apache-2.0

// -*-Mode: C++;-*- // technically C99

#ifndef level0_debug_h
#define level0_debug_h

//*****************************************************************************
// level zero includes
//*****************************************************************************

#include <ze_api.h>



//*****************************************************************************
// interface functions
//*****************************************************************************

const char *
ze_result_to_string
(
  ze_result_t result
);

#endif
