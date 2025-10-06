// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: Apache-2.0

// -*-Mode: C++;-*- // technically C99

//******************************************************************************
// system includes
//******************************************************************************

#define _GNU_SOURCE

#include <assert.h>
#include <pthread.h>
#include <string.h>



//******************************************************************************
// local includes
//******************************************************************************

#include "../../cct/cct.h"

#include "../../messages/errors.h"

#include "../../../common/lean/placeholders.h"
#include "gpu-op-placeholders.h"



//******************************************************************************
// macros
//******************************************************************************

#define SET_LOW_N_BITS(n, type) (~(((type) ~0) << n))


//******************************************************************************
// public data
//******************************************************************************

gpu_op_placeholder_flags_t gpu_op_placeholder_flags_none = 0;

gpu_op_placeholder_flags_t gpu_op_placeholder_flags_all =
  SET_LOW_N_BITS(gpu_placeholder_type_count, gpu_op_placeholder_flags_t);




//******************************************************************************
// interface operations
//******************************************************************************

ip_normalized_t
gpu_op_placeholder_ip
(
 gpu_placeholder_type_t type
)
{
  switch(type) {
  #define CASE(N) case gpu_placeholder_type_##N: return get_placeholder_norm(hpcrun_placeholder_gpu_##N);
  CASE(copy)
  CASE(copyin)
  CASE(copyout)
  CASE(alloc)
  CASE(delete)
  CASE(kernel)
  CASE(memset)
  CASE(sync)
  CASE(trace)
  CASE(scratch_alloc)
  CASE(scratch_free)
  CASE(scratch_async_reclaim)
  CASE(scratch_illegal)
  CASE(runtime)
  CASE(kernel_anon)
  CASE(paging)
  #undef CASE
  }
  assert(false && "Invalid GPU placeholder type!");
  hpcrun_terminate();
}


cct_node_t *
get_placeholder_node
(
  uint64_t correlation_id,
  gpu_placeholder_type_t pht,
  ip_normalized_t *kernel_ip
)
{
  gpu_cid_map_info_t *info = gpu_cid_map_find(correlation_id);
  assert(info);

  cct_node_t *ph = hpcrun_cct_insert_ip_norm(info->node, gpu_op_placeholder_ip(pht), true);

  *kernel_ip = info->kernel_ip;

  return ph;
}
