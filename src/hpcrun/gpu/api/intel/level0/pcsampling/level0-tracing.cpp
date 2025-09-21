// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: BSD-3-Clause

// -*-Mode: C++;-*-

//*****************************************************************************
// local includes
//*****************************************************************************

#include "level0-tracing.hpp"
#include "pcsampling-api-receiver.hpp"


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
  ze_result_t status = pcsampling::callZelTracerSetPrologues(tracer, &prologue, dispatch);
  level0_check_result(status, __LINE__);

  // Set the epilogue callbacks
  status = pcsampling::callZelTracerSetEpilogues(tracer, &epilogue, dispatch);
  level0_check_result(status, __LINE__);

  // Enable the tracer
  status = pcsampling::callZelTracerSetEnabled(tracer, true, dispatch);
  level0_check_result(status, __LINE__);
}


//******************************************************************************
// interface operations
//******************************************************************************

bool
level0CreateTracer
(
  ZeCollector* collector,
  const struct hpcrun_foil_appdispatch_level0* dispatch
)
{
  // Configure the tracer descriptor.
  // The pointer provided to pUserData here will be passed as the
  // 'global_user_data' argument to all callbacks associated with this tracer.
  zel_tracer_desc_t tracer_desc = {
    ZEL_STRUCTURE_TYPE_TRACER_EXP_DESC,  // [in] Must be this value
    nullptr,                             // [in][optional] No extension structures used
    /* .pUserData = */ collector         // [in] User context passed to callbacks
  };

  // Create the tracer
  ze_result_t status = pcsampling::callZelTracerCreate(&tracer_desc, &tracer_, dispatch);
  if (status != ZE_RESULT_SUCCESS) {
    std::cerr << "[WARNING] Unable to create Level Zero tracer" << std::endl;
    return false;
  }

  configureTracerCallbacks(tracer_, dispatch);
  return true;
}

void
level0DestroyTracer
(
  const struct hpcrun_foil_appdispatch_level0* dispatch
)
{
  if (tracer_ == nullptr) {
    return;  // Nothing to destroy
  }

  try {
    // Disable the tracer
    ze_result_t status = pcsampling::callZelTracerSetEnabled(tracer_, false, dispatch);
    level0_check_result(status, __LINE__);

    // Destroy the tracer
    status = pcsampling::callZelTracerDestroy(tracer_, dispatch);
    if (status != ZE_RESULT_SUCCESS) {
      std::cerr << "[WARNING] Failed to destroy Level Zero tracer: " 
                << ze_result_to_string(status) << std::endl;
    } else {
      tracer_ = nullptr;
    }
  } catch (const std::exception& e) {
    std::cerr << "[ERROR] Exception in level0DestroyTracer: " << e.what() << std::endl;
  }
}
