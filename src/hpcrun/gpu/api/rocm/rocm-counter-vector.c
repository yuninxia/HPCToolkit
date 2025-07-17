// SPDX-FileCopyrightText: 2024 Rice University
// SPDX-FileCopyrightText: 2024 Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: BSD-3-Clause

//******************************************************************************
// hpctoolkit includes
//******************************************************************************

#include "../../../decl-init-cast.h"

#include "rocm-counter-vector.h"


//******************************************************************************
// debugging
//******************************************************************************

#define DEBUG 0

#include "../../../gpu/common/gpu-print.h"



//*****************************************************************************
// type declarations
//*****************************************************************************

typedef struct rocm_counter_vector_t {
  rocprofiler_counter_id_t *counters;
  uint64_t entries;
  uint64_t max_entries;
} rocm_counter_vector_t;



//*****************************************************************************
// private operations
//*****************************************************************************


rocm_counter_vector_t *
rocm_counter_vector_create
(
  uint64_t max_entries
)
{
  DECL_INIT_CAST(rocm_counter_vector_t *, vector,
                 malloc(sizeof(rocm_counter_vector_t)));

  vector->counters = (rocprofiler_counter_id_t *)
    malloc(sizeof(rocprofiler_counter_id_t) * max_entries);
  vector->max_entries = max_entries;
  vector->entries = 0;

  return vector;
}


void
rocm_counter_vector_append
(
  rocm_counter_vector_t *vector,
  rocprofiler_counter_id_t id
)
{
  vector->counters[vector->entries++] = id;
}


rocprofiler_counter_id_t *
rocm_counter_vector_data
(
  rocm_counter_vector_t *vector
)
{
  return vector->counters;
}


uint64_t
rocm_counter_vector_size
(
  rocm_counter_vector_t *vector
)
{
   return vector->entries;
}


void
rocm_counter_vector_delete
(
  rocm_counter_vector_t *vector
)
{
  if (vector) {
    free(vector->counters);
    free(vector);
  }
}


void
rocm_counter_vector_dump
(
  rocm_counter_vector_t *vector
)
{
  if (vector) {
    for (int i=0; i < vector->entries; i++) {
      fprintf(stderr, "  counter id=%ld\n", vector->counters[i].handle);
    }
  }
}
