// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: Apache-2.0

/// @file rocm-pauseresume.h
/// @brief This file defines the interface for pausing and resuming monitoring of ROCm applications.

#ifndef rocm_pauseresume_h
#define rocm_pauseresume_h

//******************************************************************************
// rocprofiler includes
//******************************************************************************

#include "rocm.h"



//******************************************************************************
// public interfaces
//******************************************************************************

/// @brief Initializes the pause/resume functionality for a ROCm context.
/// @param primary_ctx Pointer to the primary ROCm context ID.
void
rocm_context_pause_resume_init
(
  rocprofiler_context_id_t *primary_ctx
);

// note: rocprofiler reports an error when stopping the pause/resume context
// in the tool finalization callback

#endif
