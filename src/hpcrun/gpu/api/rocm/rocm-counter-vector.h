// SPDX-FileCopyrightText: 2024 Rice University
// SPDX-FileCopyrightText: 2024 Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: BSD-3-Clause

/// @file rocm-counter-vector.h
/// @brief This file contains the interface for the ROCm counter vector.

#ifndef rocm_counter_vector_h
#define rocm_counter_vector_h

//*****************************************************************************
// system includes
//*****************************************************************************

#include <stdint.h>



//*****************************************************************************
// hpctoolkit includes
//*****************************************************************************

#include "rocm.h"



//*****************************************************************************
// type declarations
//*****************************************************************************

/// @brief Represents a vector of ROCm counter IDs.
typedef struct rocm_counter_vector_t rocm_counter_vector_t;



//*****************************************************************************
// interface operations
//*****************************************************************************

/// @brief Creates a new ROCm counter vector.
/// @param max_entries The maximum number of entries the vector can hold.
/// @return A pointer to the newly created counter vector.
rocm_counter_vector_t *
rocm_counter_vector_create
(
  uint64_t max_entries
);


/// @brief Appends a counter ID to the ROCm counter vector.
/// @param vector The counter vector to append to.
/// @param id The counter ID to append.
void
rocm_counter_vector_append
(
  rocm_counter_vector_t *vector,
  rocprofiler_counter_id_t id
);


/// @brief Gets the underlying data array of the ROCm counter vector.
/// @param vector The counter vector.
/// @return A pointer to the beginning of the counter ID array.
rocprofiler_counter_id_t *
rocm_counter_vector_data
(
  rocm_counter_vector_t *vector
);


/// @brief Gets the current size of the ROCm counter vector.
/// @param vector The counter vector.
/// @return The current number of counter IDs in the vector.
uint64_t
rocm_counter_vector_size
(
  rocm_counter_vector_t *vector
);


/// @brief Deletes the ROCm counter vector.
/// @param vector The counter vector to delete.
void
rocm_counter_vector_delete
(
  rocm_counter_vector_t *vector
);


/// @brief Dumps the contents of the ROCm counter vector.
/// @param vector The counter vector to dump.
void
rocm_counter_vector_dump
(
  rocm_counter_vector_t *vector
);

#endif
