// SPDX-FileCopyrightText: 2002-2024 Rice University
// SPDX-FileCopyrightText: 2024 Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: BSD-3-Clause

// -*-Mode: C++;-*-

//*****************************************************************************
// local includes
//*****************************************************************************

#include "level0-context.hpp"


//******************************************************************************
// interface operations
//******************************************************************************

ze_context_handle_t
zeroCreateContext
(
  ze_driver_handle_t driver
)
{
  ze_context_handle_t context = nullptr;
  ze_context_desc_t cdesc = {
    ZE_STRUCTURE_TYPE_CONTEXT_DESC,
    nullptr,
    0
  };
  ze_result_t status = zeContextCreate(driver, &cdesc, &context);
  level0_check_result(status, __LINE__);
  return context;
}