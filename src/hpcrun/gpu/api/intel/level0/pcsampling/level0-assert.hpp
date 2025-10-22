// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: Apache-2.0

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

#endif // LEVEL0_ASSERT_H
