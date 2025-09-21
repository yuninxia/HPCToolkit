// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: BSD-3-Clause

// -*-Mode: C++;-*-

//*****************************************************************************
// system includes
//*****************************************************************************

#include <iostream>

//*****************************************************************************
// local includes
//*****************************************************************************

#include "level0-context.hpp"
#include "pcsampling-api-receiver.hpp"


//******************************************************************************
// interface operations
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
  ze_context_desc_t cdesc = {
    ZE_STRUCTURE_TYPE_CONTEXT_DESC, // type
    nullptr,                        // pNext
    0                               // flags
  };

  ze_result_t status = pcsampling::callZeContextCreate(driver, &cdesc, &context, dispatch);
  if (status != ZE_RESULT_SUCCESS) {
    std::cerr << "[ERROR] Failed to create Level Zero context at line " << __LINE__
              << ": " << ze_result_to_string(status) << std::endl;
    return nullptr;
  }

  return context;
}
