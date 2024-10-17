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
// global variables
//*****************************************************************************

zel_tracer_handle_t tracer_ = nullptr;


//******************************************************************************
// interface operations
//******************************************************************************

void
zeroEnableTracing
(
  zel_tracer_handle_t tracer
)
{
  zet_core_callbacks_t prologue = {};
  zet_core_callbacks_t epilogue = {};

  epilogue.Module.pfnCreateCb = zeModuleCreateOnExit;
  prologue.Module.pfnDestroyCb = zeModuleDestroyOnEnter;
  epilogue.Kernel.pfnCreateCb = zeKernelCreateOnExit;
  prologue.CommandList.pfnAppendLaunchKernelCb = zeCommandListAppendLaunchKernelOnEnter;
  epilogue.CommandList.pfnAppendLaunchKernelCb = zeCommandListAppendLaunchKernelOnExit;
  epilogue.CommandList.pfnCreateImmediateCb = zeCommandListCreateImmediateOnExit;

  ze_result_t status = ZE_RESULT_SUCCESS;
  status = zelTracerSetPrologues(tracer, &prologue);
  level0_check_result(status, __LINE__);
  status = zelTracerSetEpilogues(tracer, &epilogue);
  level0_check_result(status, __LINE__);
  status = zelTracerSetEnabled(tracer, true);
  level0_check_result(status, __LINE__);

  tracer_ = tracer;
}

void
zeroDisableTracing
(
  void
)
{
  ze_result_t status = zelTracerSetEnabled(tracer_, false);
  level0_check_result(status, __LINE__);
}

zel_tracer_handle_t
zeroCreateTracer
(
  ZeCollector* collector
)
{
  zel_tracer_desc_t tracer_desc = {
    ZEL_STRUCTURE_TYPE_TRACER_EXP_DESC,  ///< [in] type of this structure
    nullptr,                             ///< [in][optional] pointer to extension-specific structure
    collector                            ///< [in] pointer passed to every tracer's callbacks
  };
  zel_tracer_handle_t tracer = nullptr;
  ze_result_t status = zelTracerCreate(&tracer_desc, &tracer);
  if (status != ZE_RESULT_SUCCESS) {
    std::cerr << "[WARNING] Unable to create Level Zero tracer" << std::endl;
    return nullptr;
  }
  return tracer;
}

void
zeroDestroyTracer
(
  zel_tracer_handle_t tracer
)
{
  if (tracer != nullptr) {
    ze_result_t status = zelTracerDestroy(tracer);
    level0_check_result(status, __LINE__);
  }
}
