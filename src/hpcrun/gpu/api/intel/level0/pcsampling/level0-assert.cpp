// SPDX-FileCopyrightText: 2002-2024 Rice University
// SPDX-FileCopyrightText: 2024 Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: BSD-3-Clause

// -*-Mode: C++;-*-

//*****************************************************************************
// local includes
//*****************************************************************************

#include "level0-assert.hpp"


//******************************************************************************
// interface operations
//******************************************************************************

void
level0_check_result
(
  ze_result_t result,
  int lineNo
)
{
  if (result == ZE_RESULT_SUCCESS) return;

  EEMSG("hpcrun: Level Zero API failed: %s",
        ze_result_to_string(result));

  exit(1);
}
