// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: Apache-2.0

/// @file rocm-callback.h
/// @brief This file defines the interface for managing ROCm callbacks.

#ifndef rocm_api_h
#define rocm_api_h

//******************************************************************************
// hpctoolkit includes
//******************************************************************************

#include "rocm.h"



//******************************************************************************
// public interfaces
//******************************************************************************

/// @brief Configures a callback for tracing the initiation of operations.
/// @param context_id The context ID for the callback.
/// @param kind The kind of tracing for the callback.
/// @param operations The operations to trace.
/// @param operations_count The number of operations to trace.
/// @param callback The callback function to be called.
/// @param callback_args Arguments to be passed to the callback function.
/// @param doc_string Documentation string for the callback.
/// @return The status of the configuration.
rocprofiler_status_t
rocm_callback_configure_initiation
(
  rocprofiler_context_id_t context_id,
  rocprofiler_callback_tracing_kind_t kind,
  rocprofiler_tracing_operation_t *operations,
  size_t operations_count,
  rocprofiler_callback_tracing_cb_t callback,
  void *callback_args,
  const char *doc_string
);


/// @brief Configures a callback for tracing the completion of operations.
/// @param context_id The context ID for the callback.
/// @param buffer_id The buffer ID associated with the callback.
/// @param kind The kind of tracing for the callback.
/// @return The status of the configuration.
rocprofiler_status_t
rocm_callback_configure_completion
(
  rocprofiler_context_id_t context_id,
  rocprofiler_buffer_id_t buffer_id,
  rocprofiler_buffer_tracing_kind_t kind
);

#endif
