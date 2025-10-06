// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: Apache-2.0

#ifndef rocm_api_h
#define rocm_api_h

/// @file rocm-api.h
/// @brief This file contains the public interface for ROCm API tracing.


//******************************************************************************
// hpctoolkit includes
//******************************************************************************

#include "rocm-regex.h"



//******************************************************************************
// public interfaces
//******************************************************************************

/// @brief Initializes the ROCm API.
///
/// This function initializes the ROCm API by setting up buffer tracing services
/// for different ROCprofiler buffer tracing kinds. It configures buffer
/// completion callbacks and configures buffer tracing for memory allocation,
/// memory copy, kernel dispatch, scratch memory, core API markers, and page
/// migration. It also configures buffer tracing for HIP runtime API operations.
/// @param context_id The ROCm context ID.
void rocm_api_init
(
  rocprofiler_context_id_t context_id
);


//******************************************************************************
// global variables
//******************************************************************************

/// @brief Shared array of GPU activity pairs for HIP runtime API operations.
extern gpu_activity_pair_t * rocm_hip_runtime_activity_vec;

/// @brief Length of the shared GPU activity pairs array.
extern int rocm_hip_runtime_activity_vec_len;

#endif
