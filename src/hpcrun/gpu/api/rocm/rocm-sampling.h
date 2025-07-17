// SPDX-FileCopyrightText: 2002-2024 Rice University
// SPDX-FileCopyrightText: 2024 Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: BSD-3-Clause

/// @file rocm-sampling.h
/// @brief This file contains the interface for initializing ROCm PC sampling.

#ifndef rocm_sampling_h
#define rocm_sampling_h



//******************************************************************************
// hpctoolkit includes
//******************************************************************************

#include "rocm.h"



//******************************************************************************
// public interfaces
//******************************************************************************

/// @brief Initializes ROCm PC sampling.
/// @param context_id The ROCm context ID for which PC sampling will be enabled.
void rocm_sampling_init
(
  rocprofiler_context_id_t context_id
);

#endif
