// SPDX-FileCopyrightText: 2002-2024 Rice University
// SPDX-FileCopyrightText: 2024 Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: BSD-3-Clause

/// @file rocm-cid.h
/// @brief This file defines the interface for managing external correlation IDs.
///
/// Each ROCm GPU operation of interest is assigned an external correlation ID so
/// that measurement data can be sent from a GPU monitoring thread back to the
/// application thread that initiated the GPU operation and attributed to the
/// calling context in which the operation was initiated.

#ifndef rocm_cid_h
#define rocm_cid_h

//******************************************************************************
// hpctoolkit includes
//******************************************************************************

#include "rocm.h"



//******************************************************************************
// public interfaces
//******************************************************************************

/// @brief Pushes a new correlation ID onto the stack.
/// @param context_id The ROCm context ID.
/// @return The new correlation ID.
uint64_t
rocm_cid_push
(
  rocprofiler_context_id_t context_id
);


/// @brief Pops a correlation ID from the stack.
/// @param context_id The ROCm context ID.
/// @return The popped correlation ID.
uint64_t
rocm_cid_pop
(
  rocprofiler_context_id_t context_id
);

#endif
