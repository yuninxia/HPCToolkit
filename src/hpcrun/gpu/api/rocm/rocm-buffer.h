// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: Apache-2.0

/// @file rocm-buffer.h
/// @brief This file defines the interface for managing ROCm buffers.

#ifndef rocm_buffer_h
#define rocm_buffer_h

//******************************************************************************
// hpctoolkit includes
//******************************************************************************

#include "rocm.h"



//******************************************************************************
// type declarations
//******************************************************************************

/// @brief Type for processing a ROCm record.
/// @param header The header of the ROCm record to process.
typedef void (*rocm_record_process_op_t)
(
  rocprofiler_record_header_t *header
);



//******************************************************************************
// public interfaces
//******************************************************************************

/// @brief Creates a ROCm buffer.
/// @param context_id The context ID for the buffer.
/// @param size The size of the buffer.
/// @param watermark The watermark for the buffer.
/// @param callback The callback function for the buffer.
/// @param callback_thread The thread for the callback function.
/// @param user_data User-provided data for the callback function.
/// @return The ID of the created buffer.
rocprofiler_buffer_id_t
rocm_buffer_create
(
  rocprofiler_context_id_t context_id,
  size_t size,
  size_t watermark,
  rocprofiler_buffer_tracing_cb_t callback,
  rocprofiler_callback_thread_t callback_thread,
  void *user_data
);


/// @brief Flushes a ROCm buffer.
/// @param buffer_id The ID of the buffer to flush.
void
rocm_buffer_flush
(
  rocprofiler_buffer_id_t buffer_id
);


/// @brief Flushes all ROCm buffers.
void
rocm_buffer_flush_all
(
  void
);


/// @brief Processes ROCm buffer headers.
/// @param headers An array of ROCm record headers.
/// @param num_headers The number of headers in the array.
/// @param drop_count The number of dropped records.
/// @param record_process The function to process each record.
/// @param user_data User-provided data for the processing function.
void
rocm_buffer_process
(
  rocprofiler_record_header_t **headers,
  size_t num_headers,
  uint64_t drop_count,
  rocm_record_process_op_t record_process,
  void *user_data
);


/// @brief Retrieves the ID of a ROCm buffer.
/// @param buffer_id The ROCm buffer ID.
/// @return The ID of the buffer as a uint64_t.
uint64_t
rocm_buffer_id
(
  rocprofiler_buffer_id_t buffer_id
);

#endif
