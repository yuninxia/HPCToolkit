// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: Apache-2.0
//
// SPDX-License-Identifier: MIT
// Copyright (C) 2024 Intel Corporation
// This file was inspired by and uses some code fragments from Intel's
// MIT-licensed pti-gpu (https://github.com/intel/pti-gpu)

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

  EEMSG("hpcrun: Level Zero API failed at line %d: %s",
        lineNo, ze_result_to_string(result));

  exit(1);
}
