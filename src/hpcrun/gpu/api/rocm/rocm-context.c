// SPDX-FileCopyrightText: 2002-2024 Rice University
// SPDX-FileCopyrightText: 2024 Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: BSD-3-Clause

//******************************************************************************
// hpctoolkit includes
//******************************************************************************

#include "rocm-context.h"


#define ROCPROFILER_CONTEXT_ZERO \
  ROCPROFILER_HANDLE_LITERAL(rocprofiler_context_id_t, 0)



//******************************************************************************
// debugging
//******************************************************************************

#define DEBUG 0

#include "../../../gpu/common/gpu-print.h"



//******************************************************************************
// macros
//******************************************************************************

// AMD uses a non-zero return code for context validity and activity
#define IS_AFFIRMATIVE(status) (status != 0)



//******************************************************************************
// public interfaces
//******************************************************************************

uint64_t
rocm_context_id
(
   rocprofiler_context_id_t context_id
)
{
  return context_id.handle;
}


rocprofiler_context_id_t
rocm_context_create
(
  void
)
{
  rocprofiler_context_id_t context_id = ROCPROFILER_CONTEXT_ZERO;

  ROCPROFILER_CALL
  (
    rocprofiler_create_context,
    (&context_id),
    "create rocprofiler context"
  );

  PRINT("rocm_context_create: context_id=0x%lx\n", rocm_context_id(context_id));
  TMSG(ROCM, "rocm_context_create: context_id=0x%lx", rocm_context_id(context_id));

  return context_id;
}


void
rocm_context_start
(
  rocprofiler_context_id_t context_id
)
{
  ROCPROFILER_CALL
  (
    rocprofiler_start_context,
    (context_id),
    "rocprofiler context start"
  );
}


void
rocm_context_stop
(
  rocprofiler_context_id_t context_id
)
{
  ROCPROFILER_CALL
  (
    rocprofiler_stop_context,
    (context_id),
    "stop rocprofiler context"
  );
}


bool
rocm_context_is_valid
(
  rocprofiler_context_id_t context_id
)
{
  int status = 0;
  ROCPROFILER_CALL
  (
    rocprofiler_context_is_valid,
    (context_id, &status),
    "context validity check"
  );

  return IS_AFFIRMATIVE(status);
}


bool
rocm_context_is_active
(
  rocprofiler_context_id_t context_id
)
{
  int status = 0;

  ROCPROFILER_CALL
  (
    rocprofiler_context_is_active,
    (context_id, &status),
    "check if rocprofiler context is active"
  );

  return IS_AFFIRMATIVE(status);
}


void
rocm_context_pause
(
  rocprofiler_context_id_t context_id
)
{
  if (rocm_context_is_active(context_id)) {
    rocm_context_stop(context_id);
  }
}


void
rocm_context_resume
(
  rocprofiler_context_id_t context_id
)
{
  if (!rocm_context_is_active(context_id)) {
    rocm_context_start(context_id);
  }
}
