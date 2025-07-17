// SPDX-FileCopyrightText: 2002-2024 Rice University
// SPDX-FileCopyrightText: 2024 Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: BSD-3-Clause

/// @file rocm-codeobject.h
/// @brief This file defines the interface for managing ROCm code objects.

#ifndef rocm_codeobject_h
#define rocm_codeobject_h

//******************************************************************************
// hpctoolkit includes
//******************************************************************************

#include "rocm.h"



//******************************************************************************
// type declarations
//******************************************************************************

/// @brief Represents a ROCm kernel code object.
typedef rocprofiler_callback_tracing_code_object_load_data_t
  rocprofiler_kernel_code_object_t;



//******************************************************************************
// public interfaces
//******************************************************************************

/// @brief Initializes the ROCm code object management.
/// @param context_id The ROCm context ID.\n/// @param buffer_id The ROCm buffer ID.
void
rocm_codeobject_init
(
  rocprofiler_context_id_t context_id,
  rocprofiler_buffer_id_t buffer_id
);

#endif
