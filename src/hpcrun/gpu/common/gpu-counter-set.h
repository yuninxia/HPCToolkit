// SPDX-FileCopyrightText: 2024 Rice University
// SPDX-FileCopyrightText: 2024 Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: BSD-3-Clause

#ifndef gpu_counter_set_h
#define gpu_counter_set_h

//*****************************************************************************
// system includes
//*****************************************************************************

#include <stdbool.h>



//*****************************************************************************
// type declarations
//*****************************************************************************

typedef struct gpu_counter_set_t gpu_counter_set_t;


typedef void (*gpu_counter_set_apply_fn_t)(
  const char *counter_name,
  void *arg
);



//*****************************************************************************
// interface operations
//*****************************************************************************

gpu_counter_set_t *
gpu_counter_set_new
(
  void
);


bool
gpu_counter_set_nonempty
(
  gpu_counter_set_t *set
);


bool
gpu_counter_set_insert
(
  gpu_counter_set_t *set,
  const char *counter_name
);


bool
gpu_counter_set_find
(
  gpu_counter_set_t *set,
  const char *counter_name
);


void
gpu_counter_set_apply
(
  gpu_counter_set_t *set,
  gpu_counter_set_apply_fn_t apply_fn,
  void *apply_arg
);


void
gpu_counter_set_delete
(
  gpu_counter_set_t *set
);


// return a set containing members of set1 not in set2
// NOTE: caller must delete the result.
gpu_counter_set_t *
gpu_counter_set_difference
(
  gpu_counter_set_t *set1,
  gpu_counter_set_t *set2
);


void
gpu_counter_set_dump
(
  gpu_counter_set_t *set,
  const char *name
);

#endif
