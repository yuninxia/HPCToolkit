// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: Apache-2.0

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
