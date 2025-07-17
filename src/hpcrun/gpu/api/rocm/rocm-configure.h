
// SPDX-FileCopyrightText: 2002-2024 Rice University
// SPDX-FileCopyrightText: 2024 Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: BSD-3-Clause

/// @file rocm-configure.h
/// @brief This file defines the configuration interface for ROCm profiling.

#ifndef rocm_configure_h
#define rocm_configure_h

//******************************************************************************
// hpctoolkit includes
//******************************************************************************

#include "rocm.h"



//******************************************************************************
// type declarations
//******************************************************************************

/// @brief Structure to hold tool data for ROCm profiling.
/// @param context_id The ROCm context ID.
/// @param buffer_id The ROCm buffer ID.
/// @param client_id Pointer to the ROCm client ID.
/// @param client_fini Function pointer for client finalization.
/// @param callback_thread The callback thread type.
typedef struct {
  rocprofiler_context_id_t context_id;
  rocprofiler_buffer_id_t buffer_id;
  rocprofiler_client_id_t *client_id;
  rocprofiler_client_finalize_t client_fini;
  rocprofiler_callback_thread_t callback_thread;
} hpctoolkit_tool_data_t;



//******************************************************************************
// global variables
//******************************************************************************

extern hpctoolkit_tool_data_t rocprofiler_tool_data;



//******************************************************************************
// interface operations
//******************************************************************************

/// @brief Configures the ROCm profiler tool.
/// @param version The version of the configuration.
/// @param runtime_version The version of the ROCm runtime.
/// @param priority The priority of the tool.
/// @param id Pointer to the ROCm client ID.
/// @return Pointer to the configuration result.
rocprofiler_tool_configure_result_t *
foilbase_rocprofiler_configure
(
  uint32_t version,
  const char *runtime_version,
  uint32_t priority,
  rocprofiler_client_id_t *id
);


/// @brief Configures the ROCm profiling environment.
void
rocm_configure
(
  void
);


/// @brief Finalizes the ROCm profiling configuration.
void
rocm_configure_fini
(
  void
);

#endif
