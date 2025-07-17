// SPDX-FileCopyrightText: 2002-2024 Rice University
// SPDX-FileCopyrightText: 2024 Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: BSD-3-Clause

//******************************************************************************
// hpctoolkit includes
//******************************************************************************

#include "../../../decl-init-cast.h"

#include "rocm-callback.h"
#include "rocm-context.h"
#include "rocm-pauseresume.h"



//******************************************************************************
// private interfaces
//******************************************************************************

static void
rocm_context_pause_resume_callback
(
  rocprofiler_callback_tracing_record_t record,
  rocprofiler_user_data_t *__unused__,
  void *user_data
)
{
  DECL_INIT_CAST(rocprofiler_context_id_t *, primary_context_id, user_data);
  if (record.kind == ROCPROFILER_CALLBACK_TRACING_MARKER_CONTROL_API) {
    switch(record.phase) {
      case ROCPROFILER_CALLBACK_PHASE_ENTER:
        if (record.operation == ROCPROFILER_MARKER_CONTROL_API_ID_roctxProfilerPause) {
          rocm_context_pause(*primary_context_id);
        }
        break;
      case ROCPROFILER_CALLBACK_PHASE_EXIT:
        if (record.operation == ROCPROFILER_MARKER_CONTROL_API_ID_roctxProfilerResume) {
          rocm_context_resume(*primary_context_id);
        }
        break;
      default:
        break;
    }
  }
}



//******************************************************************************
// public interfaces
//******************************************************************************

void
rocm_context_pause_resume_init
(
  rocprofiler_context_id_t *primary_context_id
)
{
  // Create a specialized (throw-away) context for handling ROCTx profiler pause and resume.
  // A separate context is used because if the context that is associated with roctxProfilerPause
  // disabled that same context, a call to roctxProfilerResume would be ignored because the
  // context that enables the callback for that API call is disabled.
  rocprofiler_context_id_t pause_resume_context_id = rocm_context_create();

  // enable callback marker tracing with only the pause/resume operations
  rocm_callback_configure_initiation
    (pause_resume_context_id,
    ROCPROFILER_CALLBACK_TRACING_MARKER_CONTROL_API, 0,0,
    rocm_context_pause_resume_callback, primary_context_id,
    "callback tracing marker control failed to configure");

  // start the pause/resume context so that it is always active
  rocm_context_start(pause_resume_context_id);
}
