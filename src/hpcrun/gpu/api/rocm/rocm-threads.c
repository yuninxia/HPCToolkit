// SPDX-FileCopyrightText: 2002-2024 Rice University
// SPDX-FileCopyrightText: 2024 Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: BSD-3-Clause

//******************************************************************************
// hpctoolkit includes
//******************************************************************************

#include "../../../libmonitor/monitor.h"
#include "../../../messages/messages.h"

#include "rocm-threads.h"



//******************************************************************************
// private interfaces
//******************************************************************************

static void
rocm_thread_precreate
(
  rocprofiler_runtime_library_t lib,
  void *tool_data
)
{
  monitor_disable_new_threads();
}


static void
rocm_thread_postcreate
(
  rocprofiler_runtime_library_t lib,
  void *tool_data
)
{
  monitor_enable_new_threads();
}



//******************************************************************************
// public interfaces
//******************************************************************************

void rocm_threads_ignore_rocm_threads
(
  void *rocprofiler_tool_data
)
{
  ROCPROFILER_CALL
  (rocprofiler_at_internal_thread_create,
    (rocm_thread_precreate, rocm_thread_postcreate,
     ROCPROFILER_LIBRARY | ROCPROFILER_HSA_LIBRARY | ROCPROFILER_HIP_LIBRARY |
     ROCPROFILER_MARKER_LIBRARY, rocprofiler_tool_data),
    "registration for rocprofiler thread notifications"
  );
}


rocprofiler_callback_thread_t
rocm_threads_create_callback_thread
(
  void
)
{
  rocprofiler_callback_thread_t thread;
  ROCPROFILER_CALL
  (
    rocprofiler_create_callback_thread,
    (&thread),
    "creating callback thread"
  );

  return thread;
}


void
rocm_threads_assign_callback_thread
(
  rocprofiler_buffer_id_t buffer_id,
  rocprofiler_callback_thread_t thread
)
{
  ROCPROFILER_CALL
  (
    rocprofiler_assign_callback_thread,
    (buffer_id, thread),
    "assignment of thread for buffer"
  );
}


rocprofiler_thread_id_t
rocm_threads_self
(
  void
)
{
  rocprofiler_thread_id_t my_thread_id;

  ROCPROFILER_CALL
  (
    rocprofiler_get_thread_id,
    (&my_thread_id),
    "get thread id"
  );

  return my_thread_id;
}
