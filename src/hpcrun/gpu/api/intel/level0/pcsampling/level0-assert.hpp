// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
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

#include <cstdlib>


//*****************************************************************************
// local includes
//*****************************************************************************

#include "../pcsampling-hpcrun-api.h"

extern "C" {
  #include "../level0-debug.h"
  #include "../../../../../messages/messages.h"
}


//******************************************************************************
// interface operations
//******************************************************************************

// DEPRECATED: Use level0_check_result_safe() instead
void
level0_check_result
(
  ze_result_t result,
  int lineNo
);

// Safe version that returns error code instead of calling exit()
pcsampling_result_t
level0_check_result_safe
(
  ze_result_t result,
  const char* context,
  const char* file,
  int line
);

// Convenience macro for safe error checking
#define LEVEL0_CHECK_SAFE(result, context) \
  level0_check_result_safe(result, context, __FILE__, __LINE__)

#endif // LEVEL0_ASSERT_H
