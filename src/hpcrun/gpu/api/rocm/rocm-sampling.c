// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: Apache-2.0

//******************************************************************************
// system includes
//******************************************************************************

#include <string.h>



//******************************************************************************
// hpctoolkit includes
//******************************************************************************

#include "../../../decl-init-cast.h"
#include "../../common/gpu-monitoring.h"

#include "rocm.h"
#include "rocm-activity.h"
#include "rocm-agent.h"
#include "rocm-buffer.h"
#include "rocm-codeobject-map.h"
#include "rocm-context.h"
#include "rocm-sampling.h"
#include "rocm-threads.h"



//******************************************************************************
// debugging
//******************************************************************************

#define DEBUG 0

#include "../../../gpu/common/gpu-print.h"



//******************************************************************************
// macros
//******************************************************************************

#define UPDATE_IF_NULL(x, newval) \
    if (x == 0) { \
      x = config_dup(newval); \
    }

#define CHOOSE_CONFIG(sc) \
  ((sc)->stochastic ? (sc)->stochastic : (sc)->host_trap)

#define PC_SAMPLING_BUFFER_SIZE_BYTES 10000
#define PC_SAMPLING_WATERMARK_IN_BYTES 5000



//******************************************************************************
// type declarations
//******************************************************************************

typedef struct {
  const rocprofiler_pc_sampling_configuration_t *stochastic;
  const rocprofiler_pc_sampling_configuration_t *host_trap;
} rocm_sampling_config_t;


typedef struct {
  rocprofiler_context_id_t context_id;
  unsigned int gpus_sampled;
} rocm_agent_op_state_t;



//******************************************************************************
// private variables
//******************************************************************************

static size_t rocm_sampling_interval;



//******************************************************************************
// private interfaces
//******************************************************************************

static void
rocm_sampling_interval_set
(
  size_t value
)
{
  rocm_sampling_interval = value;
}


static size_t
rocm_sampling_interval_get
(
  void
)
{
  return rocm_sampling_interval;
}


static rocprofiler_pc_sampling_configuration_t *
config_dup
(
  const rocprofiler_pc_sampling_configuration_t *og
)
{
  rocprofiler_pc_sampling_configuration_t *copy =
    malloc(sizeof(rocprofiler_pc_sampling_configuration_t));
  memcpy(copy, og, sizeof(rocprofiler_pc_sampling_configuration_t));
  return copy;
}


static void
rocm_sampling_callback
(
  rocprofiler_context_id_t context,
  rocprofiler_buffer_id_t buffer_id,
  rocprofiler_record_header_t **headers,
  size_t num_headers,
  void *user_data,
  uint64_t drop_count
)
{
  PRINT("rocm_sampling_callback: %ld sampling records delivered, %ld sampled dropped\n",
        num_headers, drop_count);

  rocm_buffer_process(headers, num_headers, drop_count,
                      rocm_activity_process, user_data);
}


static size_t
rocm_sampling_interval_for_config
(
  const rocprofiler_pc_sampling_configuration_t *config
)
{
  // if min_interval == max_interval, PC sampling is already configured. in this
  // case, use the configured interval. otherwise, use the configured interval
  size_t interval = (config->min_interval == config->max_interval) ?
    config->min_interval : rocm_sampling_interval_get();

  PRINT("rocm_sampling_interval_for_config: min=%ld, max=%ld, interval=%ld\n",
        config->min_interval, config->max_interval, interval);

  return interval;
}


static void
rocm_sampling_agent_configure
(
  rocprofiler_context_id_t context_id,
  rocprofiler_agent_t *agent,
  const rocprofiler_pc_sampling_configuration_t *config
)
{
  // create a thread for this GPU agent
  rocprofiler_callback_thread_t sampling_callback_thread =
    rocm_threads_create_callback_thread();

  // create a buffer for this GPU agent
  rocprofiler_buffer_id_t buffer_id =
    rocm_buffer_create(context_id, PC_SAMPLING_BUFFER_SIZE_BYTES,
                        PC_SAMPLING_WATERMARK_IN_BYTES,
                        rocm_sampling_callback, sampling_callback_thread, 0);

  size_t interval = rocm_sampling_interval_for_config(config);

  PRINT("    rocm_sampling_agent_configure: context_id=0x%lx, agent_name=%s, agent_id=0x%lx, "
        "config=0x%p method=%d, unit=%d, interval=%ld, bid=%lx\n",
        rocm_context_id(context_id), agent->name, rocm_agent_get_id(agent), config, config->method,
        config->unit, interval, rocm_buffer_id(buffer_id));

  ROCPROFILER_CALL
  (
    rocprofiler_configure_pc_sampling_service,
    (context_id, agent->id, config->method, config->unit,
     interval, buffer_id CONFIGURE_PC_SAMPLING_SERVICE_FLAG_VALUE),
    "configure sampling service for GPU agent"
  );
}


static void
rocm_agent_sampling_config_op
(
  rocprofiler_agent_t *agent,
  const rocprofiler_pc_sampling_configuration_t *agent_config, // pointer to value in stack
  void *user_data
)
{
  DECL_INIT_CAST(rocm_sampling_config_t *, sc, user_data);

  PRINT("    rocm_agent_sampling_config_op: config=0x%p config_method=%x\n",
        agent_config, agent_config->method);

  // NOTE: these configs are pointers into the stack
  // so they must be copied by value into user_data
  switch(agent_config->method) {
    case ROCPROFILER_PC_SAMPLING_METHOD_STOCHASTIC:
      UPDATE_IF_NULL(sc->stochastic, agent_config);
      break;
    case ROCPROFILER_PC_SAMPLING_METHOD_HOST_TRAP:
      UPDATE_IF_NULL(sc->host_trap, agent_config);
      break;
    default:
      break;
  }
}


static void
rocm_agent_op
(
  rocprofiler_agent_t *agent,
  void *user_data
)
{
  if (strcmp(agent->name, "gfx90a") != 0 &&
      strcmp(agent->name, "gfx942") != 0) {
    PRINT("rocm_agent_op: skip sampling on agent '%s' (id=0x%lx)\n",
          agent->name, rocm_agent_get_id(agent));
    return;
  }

  PRINT("rocm_agent_op: consider sampling on agent '%s' (id=0x%lx)\n",
        agent->name, rocm_agent_get_id(agent));

  DECL_INIT_CAST(rocm_agent_op_state_t *, aos, user_data);
  rocm_sampling_config_t sampling_config;
  memset(&sampling_config, 0, sizeof(sampling_config));

  rocm_agent_sampling_config_apply
    (agent, rocm_agent_sampling_config_op, &sampling_config);

  const rocprofiler_pc_sampling_configuration_t *config = 0;

  if (gpu_monitoring_instruction_sampling_is_enabled(GPU_INSTRUCTION_SAMPLING_SW)) {
    if (sampling_config.host_trap) {
      config = sampling_config.host_trap;
    } else {
      fprintf(stderr, "FATAL: hpcrun: ROCm: software PC sampling selected (pc=sw) but unavailable.\n");
      exit(-1);
    }
  } else if (gpu_monitoring_instruction_sampling_is_enabled(GPU_INSTRUCTION_SAMPLING_HW)) {
     if (sampling_config.stochastic) {
      config = sampling_config.stochastic;
    } else {
      fprintf(stderr, "FATAL: hpcrun: ROCm: hardware PC sampling selected (pc=hw) but unavailable.\n");
      exit(-1);
    }
  } else if (gpu_monitoring_instruction_sampling_is_enabled(GPU_INSTRUCTION_SAMPLING_UNSPECIFIED)) {
    if (sampling_config.host_trap) {
      config = sampling_config.host_trap;
    } else {
      fprintf(stderr, "FATAL: hpcrun: ROCm: default PC sampling selected but (pc=sw) unavailable.\n");
      exit(-1);
    }
  }

  PRINT("  rocm_agent_op: choose config: stochastic=0x%p, host_trap=0x%p, choice=0x%p\n",
        sampling_config.stochastic, sampling_config.host_trap, config);

  if (config) {
    // enable PC sampling for this GPU agent
    rocm_sampling_agent_configure(aos->context_id, agent, config);

    aos->gpus_sampled++; // sampling enabled for another GPU

    PRINT("rocm_agent_op: sampling enabled on agent '%s' (id=0x%lx)\n",
        agent->name, rocm_agent_get_id(agent));
  }
}



//******************************************************************************
// public interfaces
//******************************************************************************

void
rocm_sampling_init
(
  rocprofiler_context_id_t context_id
)
{
  uint32_t sample_period = gpu_monitoring_instruction_sampling_period_get();

  if (sample_period == GPU_SAMPLING_PERIOD_UNSPECIFIED) return;

  setenv("ROCPROFILER_PC_SAMPLING_BETA_ENABLED", "1", 1);

  rocm_sampling_interval_set(sample_period);

  rocm_agent_op_state_t data;
  data.context_id = context_id;
  data.gpus_sampled = 0;
  rocm_agent_apply(rocm_agent_op, &data);

  PRINT("rocm_sampling_init: sampling enabled on %d GPUs\n",
        data.gpus_sampled);
}
