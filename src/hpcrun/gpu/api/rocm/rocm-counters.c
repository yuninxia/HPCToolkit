// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: Apache-2.0

//******************************************************************************
// system includes
//******************************************************************************

#include <ctype.h>
#include <stdio.h>



//******************************************************************************
// hpctoolkit includes
//******************************************************************************

#include "../../../decl-init-cast.h"
#include "../../../../common/diagnostics.h"
#include "../../../../common/lean/placeholders.h"
#include "../../common/gpu-counter-set.h"
#include "../../gpu-metrics.h"

#include "rocm-activity.h"
#include "rocm-agent.h"
#include "rocm-agent-profile-map.h"
#include "rocm-buffer.h"
#include "rocm-counters.h"
#include "rocm-counter-set.h"
#include "rocm-counter-vector.h"
#include "rocm-symbol-map.h"
#include "rocm-threads.h"



//******************************************************************************
// debugging
//******************************************************************************

#define DEBUG 0

#include "../../../gpu/common/gpu-print.h"



//******************************************************************************
// macros
//******************************************************************************

#define ROCM_COUNTERS_BUFFER_BYTES           4096
#define ROCM_COUNTERS_BUFFER_WATERMARK_BYTES 2048

#define CTR_NAME_SIZE 512
#define CTR_DESC_SIZE 4096



//******************************************************************************
// type declarations
//******************************************************************************

typedef struct rocm_agent_counters_op_state_t {
  rocm_counter_vector_t *vector;
  gpu_counter_set_t *supported;
} rocm_agent_counters_op_state_t;


typedef struct rocm_agent_counters_list_state_t {
  void *vector;
} rocm_agent_counters_list_state_t;


typedef struct rocm_counters_alloc_metrics_info_t {
  int usable_counters;
  const char **counter_name;
  const char **counter_desc;
} rocm_counters_alloc_metrics_info_t;


typedef struct rocm_counters_list_data_t {
  rocm_counter_displayfn_t display_fn;
  const char *counter_prefix;
  FILE *output;
  gpu_counter_set_t *gpu_counters_seen; // used to filter out multiple agents with same gpu type
} rocm_counters_list_data_t;


typedef struct rocm_counters_missing_t {
  unsigned int count;
} rocm_counters_missing_t;


typedef struct rocm_counters_completion_callback_state_t {
  uint64_t ext_correlation_id;
  uint64_t int_correlation_id;
  ip_normalized_t kernel_first_pc;
} rocm_counters_completion_callback_state_t;



//******************************************************************************
// private variables
//******************************************************************************

static int first_rocm_ctr_metric_id = 0;

static gpu_counter_set_t *rocm_counter_name_set;



//******************************************************************************
// private operations
//******************************************************************************

static bool
is_duplicate_counter
(
  rocprofiler_counter_info_v0_t *info,
  void *user_data
)
{
  DECL_INIT_CAST(rocm_counters_list_data_t *, list_data, user_data);
  bool duplicate = !gpu_counter_set_insert(list_data->gpu_counters_seen, info->name);

  PRINT("is_duplicate(%p, %s) = %d\n", list_data->gpu_counters_seen, info->name, duplicate);

  return duplicate;
}


static bool
parenthetical_term_contains_counter
(
   const char **expr
)
{
  const char *front = *expr;
  const char *end;
  end = front;
  int parens = 0;
  while (*end) {
    if (*end == ')') parens--;
    if (*end == '(') parens++;
    if (*end == 0 || parens == 0) break;
    end++;
  }
  while (front != end) {
    if (isupper(*front++)) return true;
  }
  *expr = end++;
  return false;
}


static bool
is_acceptable_derived_counter_expression
(
  const char *expr
)
{
  if (expr == 0) return true;

  for(;;) {
    // look for a division
    expr = strstr(expr, "/");
    if (expr == 0) break;
    // first character is a "/"
    char nextchar = *++expr;
    if (nextchar == 0) break; // nothing left
    if (isdigit(nextchar)) continue; // this is a divide by constant: safe
    if (islower(nextchar)) continue; // AMD uses lower case names for constants
    if (isupper(nextchar)) return false; // AMD uses upper case names for counters
    if (nextchar == '(') {
      if (parenthetical_term_contains_counter(&expr)) return false;
    }
  }
  return true;
}


static bool
is_acceptable_counter
(
  rocprofiler_counter_info_v0_t *info
)
{
  if (info->is_constant) return false;
  if (info->is_derived == false) return true;
  return is_acceptable_derived_counter_expression(info->expression);
}


static const char *
bool_to_string
(
  int val
)
{
  return val ? "true" : "false";
}


static uint64_t __attribute__((unused))
rocm_counter_id
(
  rocprofiler_counter_id_t id
)
{
  return id.handle;
}


static rocprofiler_counter_id_t
rocm_counter_instance_to_counter
(
  rocprofiler_counter_instance_id_t instance_id
)
{
  rocprofiler_counter_id_t counter_id;

  ROCPROFILER_CALL
  (
    rocprofiler_query_record_counter_id, (instance_id, &counter_id),
    "rocprofiler_query_record_counter_id"
  );

  return counter_id;
}


static size_t
rocm_counter_dim_pos
(
  rocprofiler_counter_instance_id_t id,
  const rocprofiler_record_dimension_info_t *dim_info
)
{
  size_t pos;

  ROCPROFILER_CALL
  (
    rocprofiler_query_record_dimension_position,
    (id, dim_info->id, &pos),
    "Could not query counter dimension position"
  );

  return pos;
}


static rocprofiler_status_t
rocm_counter_dump_dims_helper
(
  rocprofiler_counter_id_t counter_id,
  const rocprofiler_record_dimension_info_t *dim_info,
  size_t num_dims,
  void *user_data
)
{
  rocprofiler_counter_instance_id_t id = *(rocprofiler_counter_instance_id_t *) user_data;
  const char *comma = "";
  for (size_t i = 0; i < num_dims; i++) {
    fprintf(stderr, "%s %s: %ld", comma, dim_info[i].name,
          rocm_counter_dim_pos(id, &dim_info[i]));
    comma = ",";
  }

  return ROCPROFILER_STATUS_SUCCESS;
}


static void
rocm_counter_dump_dims
(
  rocprofiler_counter_id_t counter,
  rocprofiler_counter_instance_id_t id
)
{
  ROCPROFILER_CALL
  (
    rocprofiler_iterate_counter_dimensions,
    (counter, rocm_counter_dump_dims_helper, &id),
    "Could not fetch dimension info"
  );
}


static rocprofiler_counter_info_v0_t
rocm_counter_info_get
(
  rocprofiler_counter_id_t counter_id
)
{
  rocprofiler_counter_info_v0_t info;

  ROCPROFILER_CALL
  (
    rocprofiler_query_counter_info,
    (counter_id, ROCPROFILER_COUNTER_INFO_VERSION_0, &info),
    "Failed querying counter info"
  );

  return info;
}


static void
rocm_counters_send
(
  uint64_t correlation_id,
  ip_normalized_t kernel_first_pc,
  const char *counter_name,
  double counter_value
)
{
  PRINT("rocm_counters_send(ecid=ox%lx, %s, %lf)\n", correlation_id,
    counter_name, counter_value);

  rocprofiler_counter_info_v0_t *counter_info;
  unsigned int index;
  if (rocm_counter_set_find(counter_name, &counter_info, &index)) {
    gpu_activity_t ga;

    ga.kind = GPU_ACTIVITY_ONE_COUNTER;
    ga.details.counter.correlation_id = correlation_id;
    ga.details.counter.kernel_first_pc = kernel_first_pc;
    ga.details.counter.metric_id = first_rocm_ctr_metric_id + index;
    ga.details.counter.value = counter_value;

    rocm_activity_send(correlation_id, &ga);
  } else {
    PRINT("rocm_counters_send: unknown counter: %s\n", counter_name);
  }
}


static void
rocm_counters_completion_callback
(
  rocprofiler_context_id_t context_id,
  rocprofiler_buffer_id_t buffer_id,
  rocprofiler_record_header_t **headers,
  size_t num_headers,
  void *user_data,
  uint64_t __unused__
)
{
  DECL_INIT_CAST(rocm_counters_completion_callback_state_t *, state, user_data);

  for (size_t i = 0; i < num_headers; ++i) {
    rocprofiler_record_header_t *header = headers[i];

    if (header->category != ROCPROFILER_BUFFER_CATEGORY_COUNTERS) continue;

    if (header->kind == ROCPROFILER_COUNTER_RECORD_PROFILE_COUNTING_DISPATCH_HEADER) {
      DECL_INIT_CAST(rocprofiler_dispatch_counting_service_record_t *, record, header->payload);

      PRINT("\t\tCounter Dispatch: DispatchId=0x%lx KernelId=0x%lx ExtCorrId=0x%lx IntCorrId=0x%lx\n",
        record->dispatch_info.dispatch_id, record->dispatch_info.kernel_id,
        record->correlation_id.external.value, record->correlation_id.internal);
        state->ext_correlation_id = record->correlation_id.external.value;
        state->int_correlation_id = record->correlation_id.internal;
        rocm_symbol_info_t *info = rocm_symbol_map_find(record->dispatch_info.kernel_id);
        state->kernel_first_pc = (info) ? info->kernel_ip : ip_normalized_NULL;
    } else if (header->kind == ROCPROFILER_COUNTER_RECORD_VALUE) {
      DECL_INIT_CAST(rocprofiler_record_counter_t *, record, header->payload);
      rocprofiler_counter_id_t counter = rocm_counter_instance_to_counter(record->id);
      rocprofiler_counter_info_v0_t counter_info __attribute__((unused)) =
        rocm_counter_info_get(counter);

      if (record->counter_value != 0.0) {
        rocm_counters_send(state->ext_correlation_id, state->kernel_first_pc, counter_info.name, record->counter_value);
      }

      PRINT("\t\tCounter Info: DispatchId=0x%lx CtrInstanceId=0x%lx"
        " CounterId=0x%lx  Name=%s Value=%lf ExtCorrId=0x%lx IntCorrId=0x%lx\n",
        record->dispatch_id, record->id, rocm_counter_id(counter),
        counter_info.name, record->counter_value, state->ext_correlation_id, state->int_correlation_id);
    }
  }
}


// callback when a kernel dispatch is enqueued in the HSA queue. 'config' is
// a return value that specifies what counters to collect for this dispatch.
static void
rocm_counters_dispatch_callback
(
  rocprofiler_dispatch_counting_service_data_t dispatch_data,
  rocprofiler_profile_config_id_t *config,
  rocprofiler_user_data_t *user_data,
  void *callback_data_args
)
{
  rocprofiler_agent_id_t agent_id = dispatch_data.dispatch_info.agent_id;
  rocprofiler_profile_config_id_t *profile =
    rocm_agent_profile_map_find(agent_id);

  PRINT("rocm_counters_dispatch_callback: agent=0x%lx profile=%p \n",
    rocm_agent_id_get_id(agent_id), profile);

  *config = *profile;
}


static void
rocm_counters_configure_dispatch
(
  rocprofiler_context_id_t context_id,
  rocprofiler_buffer_id_t buffer_id
)
{
  ROCPROFILER_CALL
  (
    rocm_configure_counting_service,
    (context_id, buffer_id, rocm_counters_dispatch_callback, 0),
    "Could not setup buffered counter service"
  );
}


static rocprofiler_buffer_id_t
rocm_counters_configure_completion
(
  rocprofiler_context_id_t context_id
)
{
   // create a thread for handling counter buffer completion
  rocprofiler_callback_thread_t counters_callback_thread =
    rocm_threads_create_callback_thread();

  // allocate state that will be used to save information from the most recent kernel
  // dispatch header between completion callbacks. this is needed because a header and
  // associated counter records are often presented in different completion callbacks.
  DECL_INIT_CAST(rocm_counters_completion_callback_state_t *,
    state, malloc(sizeof(rocm_counters_completion_callback_state_t)));
  state->ext_correlation_id = 0;
  state->int_correlation_id = 0;
  state->kernel_first_pc = ip_normalized_NULL;

  // create a buffer for reporting counter values
  rocprofiler_buffer_id_t buffer_id =
    rocm_buffer_create(context_id, ROCM_COUNTERS_BUFFER_BYTES,
                        ROCM_COUNTERS_BUFFER_WATERMARK_BYTES,
                        rocm_counters_completion_callback, counters_callback_thread, state);

  return buffer_id;
}


static void
rocm_agent_counter_op
(
  rocprofiler_agent_t *agent,
  uint64_t num_counters,
  rocprofiler_counter_id_t *counter_id,
  void *user_data
)
{
  rocprofiler_counter_info_v0_t info = rocm_counter_info_get(*counter_id);

  PRINT("  agent name %s counter='%s'\n"
      "    desc='%s'\n"
      "    block='%s'\n"
      "    expr='%s'\n"
      "    const=%s derived=%s\n",
      agent->name, info.name, info.description, info.block, info.expression,
      bool_to_string(info.is_constant), bool_to_string(info.is_derived));

  if (is_acceptable_counter(&info) == false) return;

  if (gpu_counter_set_find(rocm_counter_name_set, info.name)) {
    DECL_INIT_CAST(rocm_agent_counters_op_state_t *, state, user_data);
    if (state->vector == 0) {
      state->vector = rocm_counter_vector_create(num_counters);
    }
    gpu_counter_set_insert(state->supported, info.name);
    rocm_counter_set_insert(&info);
    rocm_counter_vector_append(state->vector, *counter_id);
  }
}


static void
rocm_agent_counters_op
(
  rocprofiler_agent_t *agent,
  void *user_data
)
{
  int *gpus_missing_counters = (int *) user_data;

  rocm_agent_counters_op_state_t state;
  state.vector = 0;
  state.supported = gpu_counter_set_new();

  PRINT("rocm_agent_counter_op: inspect counters on agent '%s' (id=0x%lx)\n",
        agent->name, rocm_agent_get_id(agent));

  PRINT("agent %s:\n", agent->name);

  rocm_agent_counter_apply(agent, rocm_agent_counter_op, &state);

  gpu_counter_set_t *missing_counters =
    gpu_counter_set_difference(rocm_counter_name_set, state.supported);

  if (gpu_counter_set_nonempty(missing_counters)) {
    fprintf(stderr,"ERROR: hpcrun: AMD GPU '%s' lacks one or more requested counters\n", agent->name);
    if (DEBUG) {
      gpu_counter_set_dump(rocm_counter_name_set, "Requested");
    }
    const char *missing_set_name = DEBUG ? "Missing" : 0; // label set in debug mode only
    gpu_counter_set_dump(missing_counters, missing_set_name);
    ++*gpus_missing_counters;
  } else {
    PRINT("counters found for agent %s:\n", agent->name);
    if (state.vector) {
  #if DEBUG
      rocm_counter_vector_dump(state.vector);
  #endif
      rocm_agent_profile_map_insert(agent->id, state.vector);

      rocm_counter_vector_delete(state.vector);
    }
  }
  gpu_counter_set_delete(state.supported);
  gpu_counter_set_delete(missing_counters);
}


static void
rocm_agent_counter_list
(
  rocprofiler_agent_t *agent,
  uint64_t num_counters,
  rocprofiler_counter_id_t *counter_id,
  void *user_data
)
{
  DECL_INIT_CAST(rocm_counters_list_data_t *, list_data, user_data);

  rocprofiler_counter_info_v0_t info = rocm_counter_info_get(*counter_id);

  PRINT("  counter='%s'\n"
        "    desc='%s'\n"
        "    block='%s'\n"
        "    expr='%s'\n"
        "    const=%s derived=%s\n",
        info.name, info.description, info.block, info.expression,
        bool_to_string(info.is_constant), bool_to_string(info.is_derived));

  if (is_acceptable_counter(&info) == false) return;

  if (is_duplicate_counter(&info, user_data)) return;

  char name[CTR_NAME_SIZE];
  char desc[CTR_DESC_SIZE];

  // assemble counter event name information
  if (info.block && info.block[0] != 0) {
    snprintf(name, CTR_NAME_SIZE, "%s%s (block=%s, derived=%s)",
            list_data->counter_prefix, info.name,
            info.block, bool_to_string(info.is_derived));
  } else {
    snprintf(name, CTR_NAME_SIZE, "%s%s (derived=%s)",
             list_data->counter_prefix, info.name,
             bool_to_string(info.is_derived));
  }
  if (info.is_derived) {
    snprintf(desc, CTR_DESC_SIZE, "%s\n\nExpression: %s",
             info.description, info.expression);
  } else {
    snprintf(desc, CTR_DESC_SIZE, "%s", info.description);
  }

  list_data->display_fn(list_data->output, name, desc);
}


static void
rocm_agent_counters_list
(
  rocprofiler_agent_t *agent,
  void *user_data
)
{
  rocm_agent_counter_apply(agent, rocm_agent_counter_list, user_data);
}


static void
suppress_improper_compiler_warnings
(
  void
)
{
  // gcc 12.3 reports an unused function warning for bool_to_string,
  // even after marking the function with __attribute__((unused)).
  // the function is used for debugging, so we always want it compiled
  // in even if debug PRINT is disabled for this file.
  // suppress the improper warning by calling it here.
  (void) bool_to_string(0);
}


static void
rocm_counters_alloc_metrics_helper
(
  rocprofiler_counter_info_v0_t *counter_info,
  unsigned int *index,
  void *arg
)
{
  DECL_INIT_CAST(rocm_counters_alloc_metrics_info_t *, info, arg);
  if (counter_info->description) {
    int i = info->usable_counters++;
    info->counter_name[i] = counter_info->name;
    info->counter_desc[i] = counter_info->description;
    // the index recorded in the set must match the position in the vector
    // the index in the set will be used to compute a metric id, which
    // will be assigned according to the order in this vector
    *index = i;
  }
}


static void
rocm_counters_alloc_metrics
(
  void
)
{
  unsigned int num_counters = rocm_counter_set_size();

  const char **counter_name = (const char **) malloc(sizeof(const char *) * num_counters);
  const char **counter_desc = (const char **) malloc(sizeof(const char *) * num_counters);

  memset(counter_name, 0, num_counters * sizeof(const char *));

  rocm_counters_alloc_metrics_info_t alloc_metrics_info;
  alloc_metrics_info.usable_counters = 0;
  alloc_metrics_info.counter_name = counter_name;
  alloc_metrics_info.counter_desc = counter_desc;

  rocm_counter_set_apply(rocm_counters_alloc_metrics_helper, &alloc_metrics_info);

  first_rocm_ctr_metric_id = gpu_metrics_GPU_CTR_enable
    (
      alloc_metrics_info.usable_counters, alloc_metrics_info.counter_name,
      alloc_metrics_info.counter_desc
    );
}



//******************************************************************************
// public interfaces
//******************************************************************************

void
rocm_counters_init
(
  rocprofiler_context_id_t context_id
)
{
  if (gpu_counter_set_nonempty(rocm_counter_name_set)) {
    int gpus_missing_counters = 0;
    // determine which counters are available for each agent. see which of those
    // match the requested counters in the rocm_counter_set. assemble a counter
    // profile for each agent.
    rocm_agent_apply(rocm_agent_counters_op, &gpus_missing_counters);

    if (gpus_missing_counters > 0) {
      fflush(0);
      exit(-1);
    }

    rocprofiler_buffer_id_t buffer_id = rocm_counters_configure_completion(context_id);

    rocm_counters_configure_dispatch(context_id, buffer_id);

    rocm_counters_alloc_metrics();
  }

  suppress_improper_compiler_warnings();
}


void
rocm_counters_list
(
  rocm_counter_displayfn_t display_fn,
  const char *counter_prefix,
  FILE *output
)
{
  rocm_counters_list_data_t list_data;

  list_data.display_fn = display_fn;
  list_data.counter_prefix = counter_prefix;
  list_data.output = output;
  list_data.gpu_counters_seen = gpu_counter_set_new();

  rocm_agent_apply(rocm_agent_counters_list, &list_data);

  gpu_counter_set_delete(list_data.gpu_counters_seen);
}


void
rocm_counter_dump_info
(
  rocprofiler_counter_instance_id_t id // counter_id_t and the dimensions
)
{
  rocprofiler_counter_id_t counter = rocm_counter_instance_to_counter(id);

  rocm_counter_dump_dims(counter, id);
}


void
rocm_counters_wanted
(
  gpu_counter_set_t *counter_name_set
)
{
  rocm_counter_name_set = counter_name_set;
}
