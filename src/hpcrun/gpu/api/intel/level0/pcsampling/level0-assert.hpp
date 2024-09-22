// SPDX-FileCopyrightText: 2002-2024 Rice University
// SPDX-FileCopyrightText: 2024 Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: BSD-3-Clause

// -*-Mode: C++;-*-

#ifndef LEVEL0_ASSERT_H
#define LEVEL0_ASSERT_H

//*****************************************************************************
// level zero includes
//*****************************************************************************

#include <level_zero/ze_api.h>


//*****************************************************************************
// system includes
//*****************************************************************************

#include <cassert>
#include <cstdlib>


//*****************************************************************************
// local includes
//*****************************************************************************

extern "C" {
  #include "../level0-debug.h"
  #include "../../../../../messages/messages.h"
}


//******************************************************************************
// interface operations
//******************************************************************************

void
level0_check_result
(
  ze_result_t result,
  int lineNo
);

#endif
