// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: Apache-2.0

//******************************************************************************
// hpctoolkit includes
//******************************************************************************

#include "../../activity/gpu-activity-channel.h"

#include "rocm-cid.h"
#include "rocm-threads.h"



//******************************************************************************
// public interfaces
//******************************************************************************

uint64_t
rocm_cid_push
(
  rocprofiler_context_id_t context_id
)
{
  rocprofiler_user_data_t rud;
  rud.value = gpu_activity_channel_generate_correlation_id();

  ROCPROFILER_CALL
  (
    rocprofiler_push_external_correlation_id,
    (context_id, rocm_threads_self(), rud),
    "correlation id push"
  );

  return rud.value;
}


uint64_t
rocm_cid_pop
(
  rocprofiler_context_id_t context_id
)
{
  rocprofiler_user_data_t rud;
  ROCPROFILER_CALL
  (
    rocprofiler_pop_external_correlation_id,
    (context_id, rocm_threads_self(), &rud),
    "correlation id pop"
  );

  return rud.value;
}
