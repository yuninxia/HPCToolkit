// SPDX-FileCopyrightText: 2002-2024 Rice University
// SPDX-FileCopyrightText: 2024 Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: BSD-3-Clause

/// @file rocm-threads.h
/// @brief This file provides thread management interfaces for ROCm.

#ifndef rocm_threads_h
#define rocm_threads_h

//******************************************************************************
// hpctoolkit includes
//******************************************************************************

#include "rocm.h"



//******************************************************************************
// public interfaces
//******************************************************************************

/// @brief Ignore the current rocm thread.
///
/// This function is called when the current thread should be ignored by
/// hpctoolkit.
/// @param rocprofiler_tool_data tool data used by rocprofiler
void
rocm_threads_ignore_rocm_threads
(
  void *rocprofiler_tool_data
);


/// @brief Create a new callback thread.
///
/// This function creates a new thread that can be used to handle
/// rocprofiler callbacks.
///
/// @return a new callback thread ID.
rocprofiler_callback_thread_t
rocm_threads_create_callback_thread
(
  void
);


/// @brief Assign a callback thread to a buffer.
///
/// Assign a callback thread to the given buffer. All events from this
/// buffer will be reported to the given callback thread.
///
/// @param buffer_id The buffer to assign the thread to.
/// @param cb_thread_id The callback thread to assign to the buffer.
void
rocm_threads_assign_callback_thread
(
  rocprofiler_buffer_id_t buffer_id,
  rocprofiler_callback_thread_t cb_thread_id
);


/// @brief Get the id of the current rocm thread.
///
/// This function returns the id of the current rocm thread.
///
/// @return The id of the current rocm thread.
rocprofiler_thread_id_t
rocm_threads_self
(
  void
);

#endif
