// SPDX-FileCopyrightText: 2002-2024 Rice University
// SPDX-FileCopyrightText: 2024 Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: BSD-3-Clause

// -*-Mode: C++;-*-

//*****************************************************************************
// local includes
//*****************************************************************************

#include "level0-tracing.hpp"


//*****************************************************************************
// local variables
//*****************************************************************************

static zel_tracer_handle_t tracer_ = nullptr;


//*****************************************************************************
// private operations
//*****************************************************************************

static void
configureTracerCallbacks
(
  zel_tracer_handle_t tracer,
  const struct hpcrun_foil_appdispatch_level0* dispatch
)
{
  // Initialize the prologue and epilogue callback structures with zero
  zet_core_callbacks_t prologue{}; 
  zet_core_callbacks_t epilogue{};

  // Set callback functions for module and kernel operations
  epilogue.Module.pfnCreateCb = zeModuleCreateOnExit;
  prologue.Module.pfnDestroyCb = zeModuleDestroyOnEnter;
  epilogue.Kernel.pfnCreateCb = zeKernelCreateOnExit;

  // Set callback functions for command list operations
  prologue.CommandList.pfnAppendLaunchKernelCb = zeCommandListAppendLaunchKernelOnEnter;
  epilogue.CommandList.pfnAppendLaunchKernelCb = zeCommandListAppendLaunchKernelOnExit;
  epilogue.CommandList.pfnCreateImmediateCb = zeCommandListCreateImmediateOnExit;

  // Set the prologue callbacks
  ze_result_t status = f_zelTracerSetPrologues(tracer, &prologue, dispatch);
  level0_check_result(status, __LINE__);

  // Set the epilogue callbacks
  status = f_zelTracerSetEpilogues(tracer, &epilogue, dispatch);
  level0_check_result(status, __LINE__);

  // Enable the tracer
  status = f_zelTracerSetEnabled(tracer, true, dispatch);
  level0_check_result(status, __LINE__);
}


//******************************************************************************
// interface operations
//******************************************************************************

bool
zeroCreateTracer
(
  ZeCollector* collector,
  const struct hpcrun_foil_appdispatch_level0* dispatch
)
{
  zel_tracer_desc_t tracer_desc = {
    ZEL_STRUCTURE_TYPE_TRACER_EXP_DESC,  // [in] Type of this structure.
    nullptr,                             // [in][optional] Pointer to extension-specific structure.
    collector                            // [in] Pointer passed to every tracer's callbacks.
  };

  // Create the tracer
  ze_result_t status = f_zelTracerCreate(&tracer_desc, &tracer_, dispatch);
  if (status != ZE_RESULT_SUCCESS) {
    std::cerr << "[WARNING] Unable to create Level Zero tracer" << std::endl;
    return false;
  }

  configureTracerCallbacks(tracer_, dispatch);
  return true;
}

void
zeroDestroyTracer
(
  const struct hpcrun_foil_appdispatch_level0* dispatch
)
{
  if (tracer_ != nullptr) {
    // Disable the tracer
    ze_result_t status = f_zelTracerSetEnabled(tracer_, false, dispatch);
    level0_check_result(status, __LINE__);

    // Destroy the tracer
    status = f_zelTracerDestroy(tracer_, dispatch);
    level0_check_result(status, __LINE__);
  }
}
