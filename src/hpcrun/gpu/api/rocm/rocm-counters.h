// SPDX-FileCopyrightText: 2002-2024 Rice University
// SPDX-FileCopyrightText: 2024 Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: BSD-3-Clause

#ifndef rocm_counters_h
#define rocm_counters_h


//******************************************************************************
// hpctoolkit includes
//******************************************************************************

#include "../../common/gpu-counter-set.h"

#include "rocm.h"



//******************************************************************************
// type declarations
//******************************************************************************

/// @brief A function pointer type for displaying ROCm counter information.
///
/// This type defines a function pointer that can be used to display information
/// about ROCm counters, including their names and descriptions.
///
/// @param out The output stream where the counter information will be written.
/// @param name The name of the counter.
/// @param desc The description of the counter.
typedef void (*rocm_counter_displayfn_t)
(
  FILE *out,
  const char *name,
  const char *desc
);



//******************************************************************************
// public interfaces
//******************************************************************************

/// @brief Initializes the ROCm counters.
///
/// This function initializes the ROCm counters, preparing them for use. It requires
/// a ROCm context ID to associate the counters with a specific ROCm context.
///
/// @param context_id The ROCm context ID.
void rocm_counters_init
(
  rocprofiler_context_id_t context_id
);


/// @brief Specifies the desired ROCm counters.
///
/// This function sets the list of ROCm counters that are desired for collection.
/// The counters are specified using a gpu_counter_set_t, which contains the names
/// of the counters.
///
/// @param counter_name_set A pointer to the set of desired counter names.
void rocm_counters_wanted
(
  gpu_counter_set_t *counter_name_set
);


/// @brief Lists the available ROCm counters.
///
/// This function lists the available ROCm counters, displaying their names and
/// descriptions using the provided display function. The output can be filtered
/// by a counter prefix.
///
/// @param display_fn A function pointer used to display counter information.
/// @param counter_prefix A prefix to filter the displayed counters.
/// @param output The output stream where the counter information will be written.
void
rocm_counters_list
(
  rocm_counter_displayfn_t display_fn,
  const char *counter_prefix,
  FILE *output
);

#endif
