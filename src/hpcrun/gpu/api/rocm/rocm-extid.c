// SPDX-FileCopyrightText: 2002-2024 Rice University
// SPDX-FileCopyrightText: 2024 Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: BSD-3-Clause


// This file implements the rocprofiler "external correlation ID".  We
// call rocprofiler_configure_external_correlation_id_request_service()
// and provide various functions we want to watch, and we receive two
// callbacks for these functions.
//
//  1. rocm_set_external_correlation_id() -- this is a synchronous
//    callback for the function, and we do an unwind and return a
//    uint64_t as an index.
//
//  2. the second callback comes via the rocprofiler_create_buffer()
//    callback after the function completes.  The value from the first
//    callback is an argument to this callback (so we can match them up).
//    This is rocm_completion_callback() in rocm-api.c
//
// See: rocprofiler-sdk/samples/external_correlation_id_request/
// and fwd.h and external_correlation.h

//******************************************************************************
// system includes
//******************************************************************************

#include <stdint.h>



//******************************************************************************
// hpctoolkit includes
//******************************************************************************

#include "../../activity/gpu-activity-channel.h"
#include "../common/gpu-cid-map.h"
#include "../../gpu-application-thread-api.h"
#include "../../../messages/messages.h"

#include "rocm.h"
#include "rocm-extid.h"
#include "rocm-utils.h"



//******************************************************************************
// rocprofiler includes
//******************************************************************************

#include "rocprofiler.h"



//******************************************************************************
// debugging
//******************************************************************************

#define DEBUG 0

#include "../../../gpu/common/gpu-print.h"



//******************************************************************************
// macros
//******************************************************************************

#define DEBUG_EXTERNAL_ID_ALL  0



//******************************************************************************
// private data
//******************************************************************************

static uint32_t ext_request_kinds[] = {
  ROCPROFILER_EXTERNAL_CORRELATION_REQUEST_HIP_RUNTIME_API,
  // ROCPROFILER_EXTERNAL_CORRELATION_REQUEST_HSA_CORE_API,
  ROCPROFILER_EXTERNAL_CORRELATION_REQUEST_KERNEL_DISPATCH,
  ROCPROFILER_EXTERNAL_CORRELATION_REQUEST_MEMORY_COPY,
  ROCPROFILER_EXTERNAL_CORRELATION_REQUEST_SCRATCH_MEMORY,
#ifdef ROCPROFILER_BUFFER_TRACING_MEMORY_ALLOCATION
  ROCPROFILER_EXTERNAL_CORRELATION_REQUEST_MEMORY_ALLOCATION,
#endif
};

static int num_ext_request_kinds =
  sizeof(ext_request_kinds) / sizeof(ext_request_kinds[0]);



//******************************************************************************
// private operations
//******************************************************************************

static int
rocm_set_external_correlation_id
(
  rocprofiler_thread_id_t    thr_id,
  rocprofiler_context_id_t   ctx_id,
  rocprofiler_external_correlation_id_request_kind_t kind,
  rocprofiler_tracing_operation_t  op,
  uint64_t                   internal_corr_id,
  rocprofiler_user_data_t *  external_corr_id,
  void *                     user_data
)
{
  uint64_t extid = gpu_activity_channel_generate_correlation_id();

  PRINT("rocm_set_external_correlation_id: kind(%d) (extid: 0x%lx)\n",
       kind, extid);

  external_corr_id->value = extid;

  TMSG(ROCM, "rocm_set_external_correlation_id: kind(%d) (extid: 0x%lx)",
       kind, extid);

  cct_node_t *api_node = gpu_application_thread_correlation_callback(extid);

  gpu_cid_map_insert(extid, api_node, ip_normalized_NULL);

  gpu_application_thread_process_activities();

  return 0;
}



//******************************************************************************
// public interfaces
//******************************************************************************

void
rocm_extid_init
(
  rocprofiler_context_id_t context_id
)
{
  TMSG(ROCM, "rocm_extid_init");

#if DEBUG_EXTERNAL_ID_ALL
  // ask for external correlation ID for everything
  ROCPROFILER_CALL
  (
    rocprofiler_configure_external_correlation_id_request_service,
    (context_id, NULL, 0, rocm_set_external_correlation_id, NULL),
     "configure external correlation id callback"
  );
#else
  ROCPROFILER_CALL
  (
    rocprofiler_configure_external_correlation_id_request_service,
    (context_id, ext_request_kinds, num_ext_request_kinds,
     rocm_set_external_correlation_id, NULL),
    "configure external correlation id callback"
  );
#endif
}
