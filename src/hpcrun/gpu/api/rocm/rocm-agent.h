// SPDX-FileCopyrightText: 2002-2024 Rice University
// SPDX-FileCopyrightText: 2024 Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: BSD-3-Clause

/// @file rocm-agent.h
/// @brief This file defines the interface for interacting with ROCm agents.

#ifndef rocm_agent_h
#define rocm_agent_h

//******************************************************************************
// hpctoolkit includes
//******************************************************************************

#include "rocm.h"



//******************************************************************************
// type declarations
//******************************************************************************

/// @brief Type for an agent operation.
/// @param agent The ROCm profiler agent.
/// @param user_data User-provided data.
typedef void (*rocm_agent_op_t)
(
  rocprofiler_agent_t *agent,
  void *user_data
);


/// @brief Type for an agent sampling configuration operation.
/// @param agent The ROCm profiler agent.
/// @param agent_config The sampling configuration for the agent.
/// @param user_data User-provided data.
typedef void (*rocm_agent_sampling_config_op_t)
(
  rocprofiler_agent_t *agent,
  const rocprofiler_pc_sampling_configuration_t *agent_config,
  void *user_data
);


/// @brief Type for an agent counter operation.
/// @param agent The ROCm profiler agent.
/// @param num_counters The number of counters.
/// @param counter Array of counter IDs.
/// @param user_data User-provided data.
typedef void (*rocm_agent_counter_op_t)
(
  rocprofiler_agent_t *agent,
  uint64_t num_counters,
  rocprofiler_counter_id_t *counter,
  void *user_data
);



//******************************************************************************
// public interfaces
//******************************************************************************

/// @brief Applies an operation to all ROCm agents.
/// @param agent_op The agent operation to apply.
/// @param user_data User-provided data to pass to the operation.
void
rocm_agent_apply
(
  rocm_agent_op_t agent_op,
  void *user_data
);


/// @brief Applies a sampling configuration operation to a ROCm agent.
/// @param agent The ROCm profiler agent.
/// @param agent_sampling_config_op The sampling configuration operation to apply.
/// @param user_data User-provided data to pass to the operation.
void
rocm_agent_sampling_config_apply
(
  rocprofiler_agent_t *agent,
  rocm_agent_sampling_config_op_t agent_sampling_config_op,
  void *user_data
);


/// @brief Applies a counter operation to a ROCm agent.
/// @param agent The ROCm profiler agent.
/// @param agent_counter_op The counter operation to apply.
/// @param user_data User-provided data to pass to the operation.
void
rocm_agent_counter_apply
(
  rocprofiler_agent_t *agent,
  rocm_agent_counter_op_t agent_counter_op,
  void *user_data
);


/// @brief Retrieves the ID of a ROCm agent.
/// @param agent The ROCm profiler agent.
/// @return The ID of the agent.
uint64_t
rocm_agent_get_id
(
  rocprofiler_agent_t *agent
);


/// @brief Retrieves the ID from a ROCm agent ID.
/// @param agent_id The ROCm profiler agent ID.
/// @return The ID associated with the agent ID.
uint64_t
rocm_agent_id_get_id
(
  rocprofiler_agent_id_t agent_id
);

#endif
