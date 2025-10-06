// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: Apache-2.0

// -*-Mode: C++;-*- // technically C99

#ifndef cuda_function_id_map_h
#define cuda_function_id_map_h

/******************************************************************************
 * system includes
 *****************************************************************************/

#include <stdint.h>

/******************************************************************************
 * local includes
 *****************************************************************************/

#include "../../../cct/cct.h"
#include "../../../utilities/ip-normalized.h"

/******************************************************************************
 * type definitions
 *****************************************************************************/

typedef struct cuda_function_id_map_entry_t cuda_function_id_map_entry_t;

/******************************************************************************
 * interface operations
 *****************************************************************************/

cuda_function_id_map_entry_t *
cuda_function_id_map_lookup
(
 uint64_t function_id
);


void
cuda_function_id_map_insert
(
 uint64_t function_id,
 ip_normalized_t pc
);


ip_normalized_t
cuda_function_id_map_entry_pc_get
(
 cuda_function_id_map_entry_t *entry
);


void
cuda_function_id_map_delete
(
 uint64_t function_id
);

#endif
