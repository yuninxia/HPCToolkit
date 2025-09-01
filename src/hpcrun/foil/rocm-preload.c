// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: Apache-2.0

//******************************************************************************
// hpctoolkit includes
//******************************************************************************

#include "common-preload.h"
#include "rocm-private.h"

//******************************************************************************
// interface operations
//******************************************************************************

HPCRUN_EXPOSED_API rocprofiler_tool_configure_result_t*
rocprofiler_configure(uint32_t version, const char* runtime_version, uint32_t priority,
                      rocprofiler_client_id_t* id) {
  return hpcrun_foil_fetch_hooks_rocm_dl()->rocprofiler_configure(
      version, runtime_version, priority, id);
}
