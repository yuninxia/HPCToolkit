// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: Apache-2.0

//******************************************************************************
// hpctoolkit includes
//******************************************************************************

#include "rocm-callback.h"



//******************************************************************************
// debugging
//******************************************************************************

#define DEBUG 0


#include "../../../gpu/common/gpu-print.h"


//******************************************************************************
// public interfaces
//******************************************************************************

rocprofiler_status_t
rocm_callback_configure_initiation
(
  rocprofiler_context_id_t context_id,
  rocprofiler_callback_tracing_kind_t kind,
  rocprofiler_tracing_operation_t *operations,
  size_t operations_count,
  rocprofiler_callback_tracing_cb_t callback,
  void *callback_args,
  const char *doc_string
)
{
  rocprofiler_status_t status =
    ROCPROFILER_CALL
    (
      rocprofiler_configure_callback_tracing_service,
      (
        context_id, kind, operations, operations_count,
        callback, callback_args
      ),
      doc_string
    );

  return status;
}


rocprofiler_status_t
rocm_callback_configure_completion
(
  rocprofiler_context_id_t context_id,
  rocprofiler_buffer_id_t buffer_id,
  rocprofiler_callback_tracing_kind_t kind
)
{
  rocprofiler_status_t status =
    ROCPROFILER_CALL
    (
      rocprofiler_configure_buffer_tracing_service,
      (
        context_id, kind, 0, 0, buffer_id
      ),
      "buffer tracing service configure"
    );

  return status;
}
