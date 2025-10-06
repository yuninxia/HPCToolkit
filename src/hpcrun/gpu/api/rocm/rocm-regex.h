// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: Apache-2.0

/// @file rocm-regex.h
/// @brief This file contains the interface for selective ROCm buffer tracing.
///
/// Use array of regular expressions to compute the array of operation indices
/// for a given buffer tracing kind.

#ifndef rocm_regex_h
#define rocm_regex_h


//******************************************************************************
// system includes
//******************************************************************************

#include <stdint.h>



//******************************************************************************
// local includes
//******************************************************************************

#include "rocprofiler.h"



//******************************************************************************
// type declarations
//******************************************************************************

/// @brief Input mapping of regular expression strings to GPU activity enums for buffer tracing operations.
/// @brief The regular expression string.
/// @brief The major part of the GPU activity enum.
/// @brief The minor part of the GPU activity enum.
typedef struct {
  const char *str;
  int major;
  int minor;
} rocm_regex_input_t;


/// @brief A pair of major and minor GPU activity numbers.
typedef struct {
  int major;
  int minor;
} gpu_activity_pair_t;



//******************************************************************************
// public interafaces
//******************************************************************************

/// @brief Computes the array of operation indices for a given buffer tracing kind and array of regular expressions.
/// @param kind The buffer tracing kind.
/// @param regex_input An array of regular expression input structures.
/// @param regex_input_len The length of the regular expression input array.
/// @param activity_vec An output array of GPU activity pairs.
/// @param activity_vec_len An output integer for the length of the GPU activity array.
/// @param ops_vec An output array of tracing operations.
/// @param ops_vec_len An output integer for the length of the tracing operations array.
/// @return 0 on success.
int
rocm_make_buffer_kind_ops
(
  rocprofiler_buffer_tracing_kind_t kind,
  rocm_regex_input_t regex_input[],
  int regex_input_len,
  gpu_activity_pair_t *activity_vec[],
  int *activity_vec_len,
  rocprofiler_tracing_operation_t *ops_vec[],
  int *ops_vec_len
);

#endif  // rocm_regex_h
