// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: BSD-3-Clause

// -*-Mode: C++;-*-

//*****************************************************************************
// local includes
//*****************************************************************************

#include "level0-context.hpp"


//******************************************************************************
// private operations
//******************************************************************************

static ze_context_desc_t
initializeContextDescriptor
(
  void
)
{
  ze_context_desc_t cdesc = {
    ZE_STRUCTURE_TYPE_CONTEXT_DESC, // type
    nullptr,                        // pNext
    0                               // flags
  };
  return cdesc;
}


//******************************************************************************
// public methods
//******************************************************************************

ze_context_handle_t
level0CreateContext
(
  ze_driver_handle_t driver,
  const struct hpcrun_foil_appdispatch_level0* dispatch
)
{
  if (driver == nullptr) {
    std::cerr << "[ERROR] Null driver handle passed to level0CreateContext" << std::endl;
    return nullptr;
  }

  ze_context_handle_t context = nullptr;
  ze_context_desc_t cdesc = initializeContextDescriptor();
  
  ze_result_t status = f_zeContextCreate(driver, &cdesc, &context, dispatch);
  level0_check_result(status, __LINE__);
  return context;
}
