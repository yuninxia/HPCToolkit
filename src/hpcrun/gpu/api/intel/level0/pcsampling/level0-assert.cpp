// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: BSD-3-Clause

// -*-Mode: C++;-*-

//*****************************************************************************
// local includes
//*****************************************************************************

#include "level0-assert.hpp"
#include "pcsampling-api-receiver.hpp"


//******************************************************************************
// interface operations
//******************************************************************************

// DEPRECATED: This function should not be used anymore as it calls exit()
// Use level0_check_result_safe() instead
void
level0_check_result
(
  ze_result_t result,
  int lineNo
)
{
  if (result == ZE_RESULT_SUCCESS) return;

  // Report error through the API instead of calling exit()
  pcsampling::reportError(PCSAMPLING_ERROR_LEVEL0_API,
                          ze_result_to_string(result),
                          __FILE__, lineNo);

  // CRITICAL: We should NOT call exit() from a dynamically loaded library!
  // This is kept temporarily for backward compatibility but should be removed
  // exit(1);  // REMOVED: Never call exit() from library code
}


pcsampling_result_t
level0_check_result_safe
(
  ze_result_t result,
  const char* context,
  const char* file,
  int line
)
{
  if (result == ZE_RESULT_SUCCESS) {
    return PCSAMPLING_SUCCESS;
  }

  // Build error message
  char error_msg[256];
  snprintf(error_msg, sizeof(error_msg), "Level Zero API failed in %s: %s",
           context ? context : "unknown context",
           ze_result_to_string(result));

  // Report error through the API
  pcsampling::reportError(PCSAMPLING_ERROR_LEVEL0_API, error_msg, file, line);

  return PCSAMPLING_ERROR_LEVEL0_API;
}
