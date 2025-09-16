// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: Apache-2.0

// This data structure is a map from a correlation id for a GPU operation
// to the calling context tree node representing the calling context where
// the GPU operation was intiated. 

// For kernel launches, we may also store a non-zero kernel_ip, which is a
// normalized instruction pointer for the first address of the kernel. For
// some GPU monitoring infrastructures, kernel_ip is available in the
// measurement data reported from the GPU. For others，it is not. We only
// store kernel_ip in this table if it is not available at the point
// measurement data is delivered.

#ifndef gpu_cid_map_h
#define gpu_cid_map_h

//*****************************************************************************
// system includes
//*****************************************************************************

#include <stdint.h>
#include <stdbool.h>



//*****************************************************************************
// hpctoolkit includes
//*****************************************************************************

#include "../../../utilities/ip-normalized.h"



//******************************************************************************
// type declarations
//******************************************************************************

typedef struct cct_node_t cct_node_t;

typedef struct gpu_cid_map_info {
  cct_node_t *node;
  ip_normalized_t kernel_ip;
} gpu_cid_map_info_t;



//*****************************************************************************
// interface operations
//*****************************************************************************

bool
gpu_cid_map_insert
(
  uint64_t correlation_id,
  cct_node_t *node,
  ip_normalized_t kernel_ip
);


gpu_cid_map_info_t *
gpu_cid_map_find
(
  uint64_t correlation_id
);


bool
gpu_cid_map_delete
(
  uint64_t correlation_id
);


#endif
