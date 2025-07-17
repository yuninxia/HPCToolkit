// SPDX-FileCopyrightText: 2002-2024 Rice University
// SPDX-FileCopyrightText: 2024 Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: BSD-3-Clause

//******************************************************************************
// hpctoolkit includes
//******************************************************************************

#include "../../../decl-init-cast.h"
#include "../../../messages/messages.h"

#include "rocm-buffer.h"
#include "rocm-callback.h"
#include "rocm-configure.h"
#include "rocm-ompt.h"
#include "rocm-threads.h"


#if defined(ROCPROFILER_OMPT_AVAILABLE)

//******************************************************************************
// macros
//******************************************************************************

#define OMPT_BUFFER_SIZE_BYTES 4096

#define OMPT_WATERMARK_IN_BYTES \
  (OMPT_BUFFER_SIZE_BYTES - (OMPT_BUFFER_SIZE_BYTES/8))

#define FOREACH_ROCPROFILER_OMPT(macro) \
  macro(ROCPROFILER_OMPT_ID_thread_begin) \
  macro(ROCPROFILER_OMPT_ID_thread_end) \
  macro(ROCPROFILER_OMPT_ID_parallel_begin) \
  macro(ROCPROFILER_OMPT_ID_parallel_end) \
  macro(ROCPROFILER_OMPT_ID_task_create) \
  macro(ROCPROFILER_OMPT_ID_task_schedule) \
  macro(ROCPROFILER_OMPT_ID_implicit_task) \
  macro(ROCPROFILER_OMPT_ID_device_initialize) \
  macro(ROCPROFILER_OMPT_ID_device_finalize) \
  macro(ROCPROFILER_OMPT_ID_device_load) \
  macro(ROCPROFILER_OMPT_ID_sync_region_wait) \
  macro(ROCPROFILER_OMPT_ID_mutex_released) \
  macro(ROCPROFILER_OMPT_ID_dependences) \
  macro(ROCPROFILER_OMPT_ID_task_dependence) \
  macro(ROCPROFILER_OMPT_ID_work) \
  macro(ROCPROFILER_OMPT_ID_masked) \
  macro(ROCPROFILER_OMPT_ID_sync_region) \
  macro(ROCPROFILER_OMPT_ID_lock_init) \
  macro(ROCPROFILER_OMPT_ID_lock_destroy) \
  macro(ROCPROFILER_OMPT_ID_mutex_acquire) \
  macro(ROCPROFILER_OMPT_ID_mutex_acquired) \
  macro(ROCPROFILER_OMPT_ID_nest_lock) \
  macro(ROCPROFILER_OMPT_ID_flush) \
  macro(ROCPROFILER_OMPT_ID_cancel) \
  macro(ROCPROFILER_OMPT_ID_reduction) \
  macro(ROCPROFILER_OMPT_ID_dispatch) \
  macro(ROCPROFILER_OMPT_ID_target_emi) \
  macro(ROCPROFILER_OMPT_ID_target_data_op_emi) \
  macro(ROCPROFILER_OMPT_ID_target_submit_emi) \
  macro(ROCPROFILER_OMPT_ID_error) \
  macro(ROCPROFILER_OMPT_ID_callback_functions)



//******************************************************************************
// debugging
//******************************************************************************

#define DEBUG 0

#include "../../../gpu/common/gpu-print.h"



//******************************************************************************
// forward declarations
//******************************************************************************

// function operation_name is used by the code in this file only if DEBUG != 0.
// keep it available for use in the debugger even if DEBUG=0.
static const char *
operation_name
(
  int operation
) __attribute__((unused));



//******************************************************************************
// private interfaces
//******************************************************************************

static const char *
operation_name
(
  int operation
)
{
  #define RETURN_STRING(x) case x: return #x;
  switch(operation) {
  FOREACH_ROCPROFILER_OMPT(RETURN_STRING)
  default:
    return "ROCPROFILER_OMPT_ILLEGAL_OPERATION_CODE";
  }
  return 0;
}


static void
rocm_ompt_callback
(
  rocprofiler_callback_tracing_record_t record,
  rocprofiler_user_data_t *user_data,
  void *callback_data
)
{
  assert(callback_data != 0);

  assert(record.kind == ROCPROFILER_CALLBACK_TRACING_OMPT);

  PRINT("rocm_ompt_callback: operation = %s\n",
    operation_name(record.operation));

  if (record.kind == ROCPROFILER_CALLBACK_TRACING_OMPT) {
    // demonstrate the use of the ompt_data_t* fields from OMPT
    // The client has its own version of those fields as well as an interface to the
    // ompt API entry points.
    DECL_INIT_CAST(rocprofiler_callback_tracing_ompt_data_t *, data, record.payload);

    switch(record.operation) {
    case ROCPROFILER_OMPT_ID_parallel_begin:
      data->args.parallel_begin.parallel_data->value = record.correlation_id.internal;
      break;
    case ROCPROFILER_OMPT_ID_parallel_end:
      data->args.parallel_end.parallel_data->value = 0;
      break;
    case ROCPROFILER_OMPT_ID_thread_begin:
      data->args.thread_begin.thread_data->value = record.thread_id;
      break;
    case ROCPROFILER_OMPT_ID_thread_end:
      data->args.thread_end.thread_data->value = 0;
      break;
    case ROCPROFILER_OMPT_ID_implicit_task:
      data->args.implicit_task.task_data->value =
        (record.phase == ROCPROFILER_CALLBACK_PHASE_ENTER) ?
        record.correlation_id.internal : 0;
      break;
    case ROCPROFILER_OMPT_ID_target_emi:
      data->args.target_emi.task_data->value =
        (record.phase == ROCPROFILER_CALLBACK_PHASE_ENTER) ?
        record.correlation_id.internal : 0;
      break;
    case ROCPROFILER_OMPT_ID_target_data_op_emi:
      data->args.target_data_op_emi.host_op_id->value =
        (record.phase == ROCPROFILER_CALLBACK_PHASE_ENTER) ?
        record.correlation_id.internal : 0;
      break;
    case ROCPROFILER_OMPT_ID_target_submit_emi:
      data->args.target_submit_emi.host_op_id->value =
        (record.phase == ROCPROFILER_CALLBACK_PHASE_ENTER) ?
        record.correlation_id.internal : 0;
      break;
    default:
      fprintf(stderr, "WARNING: hpcrun: unrecognized OMPT op %d\n",
        record.operation);
      break;
    }
  }
}



//******************************************************************************
// public interfaces
//******************************************************************************

void
rocm_ompt_init
(
  rocprofiler_context_id_t context_id
)
{
#if 0
  // TODO: Do we really need OMPT buffer tracing here or will this happen through
  // the OMPT interface?

  // create a thread for OMPT monitoring
  rocprofiler_callback_thread_t ompt_callback_thread =
    rocm_threads_create_callback_thread();

  // create a trace buffer for OMPT monitoring
  rocprofiler_buffer_id_t buffer_id =
    rocm_buffer_create(context_id, OMPT_BUFFER_SIZE_BYTES,
                        OMPT_WATERMARK_IN_BYTES,
                        rocm_ompt_buffer_callback, ompt_callback_thread, 0);
#endif

  rocm_callback_configure_initiation
    (context_id, ROCPROFILER_CALLBACK_TRACING_OMPT, NULL,
     0, rocm_ompt_callback, &rocprofiler_tool_data,
     "failed to configure ompt callback tracing service");
}

#else // !defined(ROCPROFILER_OMPT_AVAILABLE)

void
rocm_ompt_init
(
  rocprofiler_context_id_t context_id
)
{
}

#endif
