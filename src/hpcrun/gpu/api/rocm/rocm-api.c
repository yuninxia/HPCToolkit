// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: Apache-2.0

//******************************************************************************
// hpctoolkit includes
//******************************************************************************

#include "rocm.h"
#include "rocm-activity.h"
#include "rocm-buffer.h"
#include "rocm-callback.h"
#include "rocm-configure.h"
#include "rocm-regex.h"
#include "rocm-threads.h"



//******************************************************************************
// debugging
//******************************************************************************

#define DEBUG 0

#define DEBUG_BUFFER_TRACING_ALL  0

#include "../../../gpu/common/gpu-print.h"



//******************************************************************************
// private data
//******************************************************************************

// For these kinds, we use all sub-operations.
//
static uint32_t buffer_tracing_kinds[] = {
#ifdef ROCPROFILER_BUFFER_TRACING_MEMORY_ALLOCATION
  ROCPROFILER_BUFFER_TRACING_MEMORY_ALLOCATION,
#endif
  ROCPROFILER_BUFFER_TRACING_MEMORY_COPY,
  ROCPROFILER_BUFFER_TRACING_KERNEL_DISPATCH,
  ROCPROFILER_BUFFER_TRACING_SCRATCH_MEMORY,

  ROCPROFILER_BUFFER_TRACING_MARKER_CORE_API,
#ifdef ROCPROFILER_PAGE_MIGRATION_AVAILABLE
  ROCPROFILER_BUFFER_TRACING_PAGE_MIGRATION,
#endif

#if 0
  ROCPROFILER_BUFFER_TRACING_HIP_COMPILER_API,
  ROCPROFILER_BUFFER_TRACING_MARKER_CONTROL_API,
  ROCPROFILER_BUFFER_TRACING_MARKER_NAME_API,
  ROCPROFILER_BUFFER_TRACING_CORRELATION_ID_RETIREMENT,  // Correlation ID in no longer in use
  ROCPROFILER_BUFFER_TRACING_RCCL_API,                   // RCCL tracing
  ROCPROFILER_BUFFER_TRACING_ROCDECODE_API,
#endif
};

static int num_buffer_tracing_kinds =
  sizeof(buffer_tracing_kinds) / sizeof(buffer_tracing_kinds[0]);


// For ROCPROFILER_BUFFER_TRACING_HIP_RUNTIME_API, we use the ops that
// match these regular expressions.
//
// Note: regex are case-insensitive, use '.*' for sequence of chars,
// and don't require '.*' at begin or end.
// Rules are applied in order, first match wins.
//
static rocm_regex_input_t hip_runtime_regex_input[] = {
#if !defined(ROCPROFILER_BUFFER_TRACING_MEMORY_ALLOCATION)
  { "alloc", GPU_ACTIVITY_MEMORY, GPU_MEM_OP_ALLOC },
  { "free", GPU_ACTIVITY_MEMORY, GPU_MEM_OP_DELETE },
#endif
  { "memset", GPU_ACTIVITY_MEMSET, GPU_MEM_UNKNOWN },

#if 0
  { "ctx.*synchroni[sz]e", GPU_ACTIVITY_SYNCHRONIZATION, GPU_SYNC_CONTEXT },
  { "event.*synchroni[sz]e", GPU_ACTIVITY_SYNCHRONIZATION, GPU_SYNC_EVENT },
  { "stream.*synchroni[sz]e", GPU_ACTIVITY_SYNCHRONIZATION, GPU_SYNC_STREAM },
  { "synchroni[sz]e", GPU_ACTIVITY_SYNCHRONIZATION, GPU_SYNC_UNKNOWN },
  { "memcpy.*AtoH", GPU_ACTIVITY_MEMCPY, GPU_MEMCPY_A2H },
  { "memcpy.*DtoD", GPU_ACTIVITY_MEMCPY, GPU_MEMCPY_D2D },
  { "memcpy.*DtoH", GPU_ACTIVITY_MEMCPY, GPU_MEMCPY_D2H },
  { "memcpy.*HtoA", GPU_ACTIVITY_MEMCPY, GPU_MEMCPY_H2A },
  { "memcpy.*HtoD", GPU_ACTIVITY_MEMCPY, GPU_MEMCPY_H2D },
  { "memcpy", GPU_ACTIVITY_MEMCPY, GPU_MEMCPY_UNK },
#endif
};
static int num_hip_runtime_regex_input =
  sizeof(hip_runtime_regex_input) / sizeof(hip_runtime_regex_input[0]);


// Shared with rocm-activity.
gpu_activity_pair_t * rocm_hip_runtime_activity_vec;
int rocm_hip_runtime_activity_vec_len;


//******************************************************************************
// private interfaces
//******************************************************************************

static void
rocm_completion_callback
(
  rocprofiler_context_id_t context,
  rocprofiler_buffer_id_t buffer_id,
  rocprofiler_record_header_t **headers,
  size_t num_headers,
  void *user_data,
  uint64_t drop_count
)
{
  TMSG(ROCM, "rocm_completion_callback");

  rocm_buffer_process(headers, num_headers, drop_count,
                      rocm_activity_process, user_data);
}


static void
rocm_completion_callback_init
(
  rocprofiler_context_id_t context_id
)
{
  TMSG(ROCM, "rocm_completion_callback_init");

  rocprofiler_tool_data.callback_thread = rocm_threads_create_callback_thread();
  rocprofiler_tool_data.buffer_id =
    rocm_buffer_create
      (context_id, 4096, 2048, rocm_completion_callback,
       rocprofiler_tool_data.callback_thread, 0);

#if DEBUG_BUFFER_TRACING_ALL
  // catch all buffer tracing events, except ID retirement
  for (int i = ROCPROFILER_BUFFER_TRACING_NONE + 1;
       i < ROCPROFILER_BUFFER_TRACING_LAST;
       i++) {
    if (i != ROCPROFILER_BUFFER_TRACING_CORRELATION_ID_RETIREMENT) {
      rocm_callback_configure_completion
        (context_id, rocprofiler_tool_data.buffer_id, i);
    }
  }

#else  // ! DEBUG_BUFFER_TRACING_ALL

  // Normal case of selected kinds/ops.
  // Kinds that use all ops.

  for (int i = 0; i < num_buffer_tracing_kinds; i++) {
    ROCPROFILER_CALL
    (
      rocprofiler_configure_buffer_tracing_service,
      (
        context_id,
        buffer_tracing_kinds[i],
        NULL,
        0,
        rocprofiler_tool_data.buffer_id
      ),
      "buffer tracing service configure"
    );
  }

  // Ops for ROCPROFILER_BUFFER_TRACING_HIP_RUNTIME_API

  rocprofiler_tracing_operation_t *ops_list;

  int num_ops;

  int ret =
    rocm_make_buffer_kind_ops
    ( ROCPROFILER_BUFFER_TRACING_HIP_RUNTIME_API,
      hip_runtime_regex_input,
      num_hip_runtime_regex_input,
      &rocm_hip_runtime_activity_vec,
      &rocm_hip_runtime_activity_vec_len,
      &ops_list,
      &num_ops
    );

  if (ret != 0) {
    EMSG("rocm_completion_callback_init: rocm_make_buffer_kind_ops() failed");
    exit(-1);
  }

  ROCPROFILER_CALL
  (
    rocprofiler_configure_buffer_tracing_service,
    (
      context_id,
      ROCPROFILER_BUFFER_TRACING_HIP_RUNTIME_API,
      ops_list,
      num_ops,
      rocprofiler_tool_data.buffer_id
    ),
    "buffer tracing service configure"
  );

#endif  // ! DEBUG_BUFFER_TRACING_ALL
}



//******************************************************************************
// public interfaces
//******************************************************************************

void
rocm_api_init
(
  rocprofiler_context_id_t context_id
)
{
  rocm_completion_callback_init(context_id);
}
