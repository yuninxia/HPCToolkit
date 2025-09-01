// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: Apache-2.0

/// @file rocm-bufferset.h
/// @brief This file defines the interface for managing a set of ROCm buffers.

#ifndef rocm_bufferset_h
#define rocm_buffer_set_h

//******************************************************************************
// hpctoolkit includes
//******************************************************************************

#include "rocm.h"



//******************************************************************************
// type declarations
//******************************************************************************

/// @brief Type for an operation on a ROCm buffer.
/// @param buffer_id The ID of the ROCm buffer.
typedef void (*rocm_buffer_op_t)
(
  rocprofiler_buffer_id_t buffer_id
);



//******************************************************************************
// public interfaces
//******************************************************************************

/// @brief Initializes the ROCm buffer set.
void rocm_bufferset_init
(
  void
);


/// @brief Inserts a ROCm buffer into the buffer set.
/// @param buffer_id The ID of the ROCm buffer to insert.
void rocm_bufferset_insert
(
  rocprofiler_buffer_id_t buffer_id
);


/// @brief Applies an operation to each buffer in the buffer set.
/// @param buffer_op The operation to apply to each buffer.
void
rocm_bufferset_apply
(
  rocm_buffer_op_t buffer_op
);

#endif
