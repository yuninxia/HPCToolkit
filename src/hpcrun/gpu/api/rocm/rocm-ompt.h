// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: Apache-2.0

/// @file rocm-ompt.h
/// @brief This file contains the interface for ROCm OMPT functions.

//******************************************************************************
// hpctoolkit includes
//******************************************************************************

#include "rocm-context.h"



//******************************************************************************
// interface operations
//******************************************************************************

/// @brief Initialize the ROCm OMPT interface.
/// @param context_id The ROCm context ID.
void
rocm_ompt_init
(
  rocprofiler_context_id_t context_id
);
