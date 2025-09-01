// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: Apache-2.0

// -*-Mode: C++;-*- // technically C99

#ifndef cuda_function_name_map_h
#define cuda_function_name_map_h

//*****************************************************************************
// system includes
//*****************************************************************************

#include <stdint.h>
#include <stdbool.h>



//*****************************************************************************
// hpctoolkit includes
//*****************************************************************************

#include "../../../utilities/ip-normalized.h"



//*****************************************************************************
// interface operations
//*****************************************************************************

void
cuda_function_name_map_init
(
  void
);


bool
cuda_function_name_map_insert
(
  const char *name,
  ip_normalized_t pc
);


ip_normalized_t
cuda_function_name_map_lookup
(
 const char *name
);


void
cuda_function_name_map_dump
(
 void
);


#endif
