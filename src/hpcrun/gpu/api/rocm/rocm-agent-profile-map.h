// SPDX-FileCopyrightText: 2024 Rice University
// SPDX-FileCopyrightText: 2024 Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: BSD-3-Clause

/// @file rocm-agent-profile-map.h
/// @brief This file defines the interface for mapping ROCm agents to their configurations for profiling.

#ifndef rocm_agent_profile_map_h
#define rocm_agent_profile_map_h

//*****************************************************************************
// system includes
//*****************************************************************************

#include <stdbool.h>



//*****************************************************************************
// hpctoolkit includes
//*****************************************************************************

#include "rocm.h"
#include "rocm-counter-vector.h"



//*****************************************************************************
// interface operations
//*****************************************************************************

/// @brief Initializes the ROCm agent profile map.
void
rocm_agent_profile_map_init
(
  void
);


/// @brief Inserts a new entry into the ROCm agent profile map.
/// @param id The ID of the ROCm agent.
/// @param vector A pointer to the counter vector associated with the agent.
/// @return True if the insertion was successful, false otherwise.
bool
rocm_agent_profile_map_insert
(
  rocprofiler_agent_id_t id,
  rocm_counter_vector_t *vector
);


/// @brief Finds the profile configuration ID for a given ROCm agent.
/// @param id The ID of the ROCm agent.
/// @return A pointer to the profile configuration ID, or NULL if not found.
rocprofiler_profile_config_id_t *
rocm_agent_profile_map_find
(
  rocprofiler_agent_id_t id
);


/// @brief Dumps the contents of the ROCm agent profile map.
void
rocm_agent_profile_map_dump
(
  void
);

#endif
