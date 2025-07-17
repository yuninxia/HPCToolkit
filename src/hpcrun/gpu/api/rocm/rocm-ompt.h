// SPDX-FileCopyrightText: 2002-2024 Rice University
// SPDX-FileCopyrightText: 2024 Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: BSD-3-Clause

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
