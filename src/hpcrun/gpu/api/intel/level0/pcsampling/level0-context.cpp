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

#include "level0-context.hpp"
#include "level0-pc-api-receiver.hpp"


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
    pcsampling::error("Null driver handle passed to level0CreateContext");
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
    pcsampling::error("Failed to create Level Zero context: %s", ze_result_to_string(status));
    return nullptr;
  }

  return context;
}
