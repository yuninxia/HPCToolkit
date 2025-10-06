// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: Apache-2.0

/// @file rocm-context.h
/// @brief This file defines the interface for managing ROCm contexts.

#ifndef rocm_context_h
#define rocm_context_h

//******************************************************************************
// hpctoolkit includes
//******************************************************************************

#include "rocm.h"



//******************************************************************************
// public interfaces
//******************************************************************************

/// @brief Creates a new ROCm context.
/// @return The ID of the created context.
rocprofiler_context_id_t
rocm_context_create
(
  void
);


/// @brief Starts a ROCm context.
/// @param context_id The ID of the context to start.
void
rocm_context_start
(
  rocprofiler_context_id_t context_id
);


/// @brief Stops a ROCm context.
/// @param context_id The ID of the context to stop.
void
rocm_context_stop
(
  rocprofiler_context_id_t context_id
);


/// @brief Checks if a ROCm context ID is valid.
/// @param context_id The ID of the context to check.
/// @return True if the context ID is valid, false otherwise.
bool
rocm_context_is_valid
(
  rocprofiler_context_id_t context_id
);


/// @brief Checks if a ROCm context is active.
/// @param context_id The ID of the context to check.
/// @return True if the context is active, false otherwise.
bool
rocm_context_is_active
(  rocprofiler_context_id_t context_id);


/// @brief Pauses a ROCm context.
/// @param context_id The ID of the context to pause.
void
rocm_context_pause
(
  rocprofiler_context_id_t context_id
);


/// @brief Resumes a ROCm context.
/// @param context_id The ID of the context to resume.
void
rocm_context_resume
(
  rocprofiler_context_id_t context_id
);


/// @brief Returns the underlying uint64_t ID of a ROCm context.
/// @param context_id The ROCm context ID.
/// @return The underlying uint64_t ID.
uint64_t
rocm_context_id
(
  rocprofiler_context_id_t context_id
);

#endif
