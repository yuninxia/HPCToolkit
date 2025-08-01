// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: Apache-2.0

// -*-Mode: C++;-*- // technically C99

#ifndef HPCRUN_GPU_GPU_CCT_H
#define HPCRUN_GPU_GPU_CCT_H

//******************************************************************************
// local includes
//******************************************************************************

#include "../../../cct/cct.h"
#include "../../../utilities/ip-normalized.h"

//******************************************************************************
// interface operations
//******************************************************************************

void
gpu_cct_insert
(
 cct_node_t *cct_node,
 ip_normalized_t ip
);


cct_node_t *
gpu_cct_insert_always
(
 cct_node_t *cct_node,
 ip_normalized_t ip
);

#endif  // HPCRUN_GPU_GPU_CCT_H
