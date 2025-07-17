// SPDX-FileCopyrightText: 2002-2024 Rice University
// SPDX-FileCopyrightText: 2024 Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: BSD-3-Clause

//******************************************************************************
// hpctoolkit includes
//******************************************************************************

#include "../../../decl-init-cast.h"

#include "rocm-agent.h"



//******************************************************************************
// debugging
//******************************************************************************

#define DEBUG 0

#include "../../../gpu/common/gpu-print.h"



//******************************************************************************
// type declarations
//******************************************************************************

typedef struct rocm_agent_apply_helper_state_t {
  rocm_agent_op_t op;
  void *user_data;
} rocm_agent_apply_helper_state_t;


typedef struct rocm_agent_sampling_config_apply_helper_state_t {
  rocprofiler_agent_t *agent;
  rocm_agent_sampling_config_op_t op;
  void *user_data;
} rocm_agent_sampling_config_apply_helper_state_t;


typedef struct rocm_agent_counter_apply_helper_state_t {
  rocprofiler_agent_t *agent;
  rocm_agent_counter_op_t op;
  void *user_data;
} rocm_agent_counter_apply_helper_state_t;



//******************************************************************************
// private interfaces
//******************************************************************************

static rocprofiler_status_t
rocm_agent_apply_helper
(
  rocprofiler_agent_version_t version,
  const void **agents,
  size_t num_agents,
  void *user_data // a pointer rocm_agents_apply_helper_state_t
)
{
  DECL_INIT_CAST(rocprofiler_agent_t **, agent, agents);
  DECL_INIT_CAST(rocm_agent_apply_helper_state_t *, state, user_data);

  assert(version == ROCPROFILER_AGENT_INFO_VERSION_0);

  for (size_t i = 0; i < num_agents; i++) {
    // 1. agent is visible to the computation according to ROCR_VISIBLE_DEVICES
    // 2. agent represents a GPU
    if (ROCM_AGENT_VISIBLE(agent[i]) && agent[i]->type == ROCPROFILER_AGENT_TYPE_GPU) {
      state->op(agent[i], state->user_data);
    }
  }

  return ROCPROFILER_STATUS_SUCCESS;
}


static rocprofiler_status_t
rocm_agent_sampling_config_apply_helper
(
  const rocprofiler_pc_sampling_configuration_t* configs,
  size_t num_configs,
  void *user_data // pointer to rocm_agent_sampling_config_apply_helper_state_t
)
{
  DECL_INIT_CAST(rocm_agent_sampling_config_apply_helper_state_t *, state, user_data);

  for (size_t i = 0; i < num_configs; i++) {
    state->op(state->agent, &configs[i], state->user_data);
  }

  return ROCPROFILER_STATUS_SUCCESS;
}


static rocprofiler_status_t
rocm_agent_counter_apply_helper
(
  rocprofiler_agent_id_t agent_id,
  rocprofiler_counter_id_t *counters,
  size_t num_counters,
  void *user_data // pointer to rocm_agent_counter_apply_helper_state_t
)
{
  DECL_INIT_CAST(rocm_agent_counter_apply_helper_state_t *, state, user_data);

  for (uint64_t i = 0; i < num_counters; i++) {
    state->op(state->agent, num_counters, &counters[i], state->user_data);
  }

  return ROCPROFILER_STATUS_SUCCESS;
}



//******************************************************************************
// public interfaces
//******************************************************************************

void
rocm_agent_apply
(
  rocm_agent_op_t agent_op,
  void *user_data
)
{
  rocm_agent_apply_helper_state_t state;
  state.op = agent_op;
  state.user_data = user_data;

  PRINT("rocm_agent_apply: begin\n");
  // This function returns the all gpu agents supporting some kind of PC sampling
  ROCPROFILER_CALL
  (
    rocprofiler_query_available_agents,
    (
      ROCPROFILER_AGENT_INFO_VERSION_0,
      &rocm_agent_apply_helper,
      sizeof(rocprofiler_agent_t),
      &state
    ),
    "query agents for PC sampling support"
  );
  PRINT("rocm_agent_apply: end\n");
}


void
rocm_agent_sampling_config_apply
(
  rocprofiler_agent_t *agent,
  rocm_agent_sampling_config_op_t agent_sampling_config_op,
  void *user_data
)
{
  rocm_agent_sampling_config_apply_helper_state_t state;
  state.agent = agent;
  state.op = agent_sampling_config_op;
  state.user_data = user_data;

  PRINT("  rocm_agent_sampling_config_apply: begin\n");

  ROCPROFILER_CALL
  (
    rocprofiler_query_pc_sampling_agent_configurations,
    (
      agent->id,
      rocm_agent_sampling_config_apply_helper,
      &state
    ),
    "query sampling configs for GPU agent"
  );

  PRINT("  rocm_agent_sampling_config_apply: end\n");
}


void
rocm_agent_counter_apply
(
  rocprofiler_agent_t *agent,
  rocm_agent_counter_op_t agent_counter_op,
  void *user_data
)
{
  rocm_agent_counter_apply_helper_state_t state;
  state.agent = agent;
  state.op = agent_counter_op;
  state.user_data = user_data;

  PRINT("  rocm_agent_counter_apply: begin\n");

  // Iterate counters available on an agent
  ROCPROFILER_CALL
  (
    rocprofiler_iterate_agent_supported_counters,
    (
      agent->id,
      rocm_agent_counter_apply_helper,
      &state
    ),
    "Could not fetch supported counters"
  );

  PRINT("  rocm_agent_counter_apply: end\n");
}


uint64_t
rocm_agent_id_get_id
(
  rocprofiler_agent_id_t agent_id
)
{
  return agent_id.handle;
}


uint64_t
rocm_agent_get_id
(
  rocprofiler_agent_t *agent
)
{
  return rocm_agent_id_get_id(agent->id);
}
