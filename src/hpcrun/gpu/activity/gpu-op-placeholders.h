// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: Apache-2.0

// -*-Mode: C++;-*- // technically C99

#ifndef gpu_op_placeholders_h
#define gpu_op_placeholders_h



//******************************************************************************
// system includes
//******************************************************************************

#include <stdint.h>



//******************************************************************************
// local includes
//******************************************************************************

#include "../../utilities/ip-normalized.h"
#include "../../cct/cct.h"
#include "../api/common/gpu-cid-map.h"



//******************************************************************************
// macros
//******************************************************************************

#define GPU_RUNTIME_PH_CID \
  ((0x1UL << 32) | (0x7f7f7f7f)) // thread 1 with correlation id 0x7f7f7f7f

#define PARTIAL_UNWIND_PH_CID \
  ((0x1UL << 32) | (0xf7f7f7f7)) // thread 1 with correlation id 0xf7f7f7f7




//******************************************************************************
// type declarations
//******************************************************************************

typedef enum gpu_placeholder_type_t {
  gpu_placeholder_type_copy    = 0, // general copy, d2d d2a, or a2d
  gpu_placeholder_type_copyin  = 1,
  gpu_placeholder_type_copyout = 2,
  gpu_placeholder_type_alloc   = 3,
  gpu_placeholder_type_delete  = 4,
  gpu_placeholder_type_kernel  = 5,
  gpu_placeholder_type_memset  = 6,
  gpu_placeholder_type_sync    = 7,
  gpu_placeholder_type_trace   = 8,
  gpu_placeholder_type_paging = 9,
  gpu_placeholder_type_runtime = 10,
  gpu_placeholder_type_kernel_anon = 11,
  gpu_placeholder_type_scratch_alloc = 12,
  gpu_placeholder_type_scratch_free = 13,
  gpu_placeholder_type_scratch_async_reclaim = 14,
  gpu_placeholder_type_scratch_illegal = 15
} gpu_placeholder_type_t;

#define gpu_placeholder_type_count 16


typedef uint32_t gpu_op_placeholder_flags_t;



//******************************************************************************
// public data
//******************************************************************************

extern gpu_op_placeholder_flags_t gpu_op_placeholder_flags_all;
extern gpu_op_placeholder_flags_t gpu_op_placeholder_flags_none;



//******************************************************************************
// interface operations
//******************************************************************************

ip_normalized_t
gpu_op_placeholder_ip
(
 gpu_placeholder_type_t type
);


cct_node_t *
get_placeholder_node
(
  uint64_t correlation_id,
  gpu_placeholder_type_t pht,
  ip_normalized_t *kernel_ip
);

#endif
