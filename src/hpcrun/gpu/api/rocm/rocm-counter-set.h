// SPDX-FileCopyrightText: 2024 Rice University
// SPDX-FileCopyrightText: 2024 Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: BSD-3-Clause

/// @file rocm-counter-set.h
/// @brief This file contains the interface the set of ROCm counters to be profiled.

#ifndef rocm_counter_set_h
#define rocm_counter_set_h

//*****************************************************************************
// system includes
//*****************************************************************************

#include <stdbool.h>



//*****************************************************************************
// system includes
//*****************************************************************************

#include "rocm.h"



//*****************************************************************************
// type declarations
//*****************************************************************************

/// @brief Pointer to a function to be applied to each counter in the set.
/// @param counter_info Pointer to the counter information.
/// @param index Pointer to the index of the counter.
typedef void (*rocm_counter_set_apply_fn_t)(
  rocprofiler_counter_info_v0_t *counter_info,
  unsigned int *index,
  void *arg
);



//*****************************************************************************
// interface operations
//*****************************************************************************

/// @brief Initializes the ROCm counter set.
void
rocm_counter_set_init
(
  void
);


/// @brief Checks if the ROCm counter set is non-empty.
/// @return True if the counter set is non-empty, false otherwise.
bool
rocm_counter_set_nonempty
(
  void
);


/// @brief Inserts a counter into the ROCm counter set.
/// @param counter_info Pointer to the counter information to insert.
/// @return True if the counter was inserted, false otherwise.
bool
rocm_counter_set_insert
(
  rocprofiler_counter_info_v0_t *counter_info
);


/// @brief Finds a counter in the ROCm counter set by name.
/// @param counter_name The name of the counter to find.
/// @param counter_info Output parameter: Pointer to the found counter information.
/// @param index Output parameter: Pointer to the index of the found counter.
/// @return True if the counter was found, false otherwise.
bool
rocm_counter_set_find
(
  const char *counter_name,
  rocprofiler_counter_info_v0_t **counter_info,
  unsigned int *index
);


/// @brief Gets the size of the ROCm counter set.
/// @return The number of counters in the set.
unsigned int
rocm_counter_set_size
(
  void
);


/// @brief Applies a function to each counter in the ROCm counter set.
/// @param apply_fn The function to apply to each counter.
/// @param apply_arg Argument to pass to the apply function.
void
rocm_counter_set_apply
(
  rocm_counter_set_apply_fn_t apply_fn,
  void *apply_arg
);


/// @brief Dumps the contents of the ROCm counter set.
void
rocm_counter_set_dump
(
  void
);

#endif
