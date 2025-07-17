// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: BSD-3-Clause

//******************************************************************************
// hpctoolkit includes
//******************************************************************************

#include "rocm-private.h"



//******************************************************************************
// macros
//******************************************************************************

#define FORALL_ROCPROFILER_FUNCTIONS(MACRO) \
  MACRO(rocprofiler_query_available_agents) \
  MACRO(rocprofiler_query_pc_sampling_agent_configurations) \
  MACRO(rocprofiler_iterate_agent_supported_counters) \
  MACRO(rocprofiler_create_profile_config) \
  MACRO(rocprofiler_configure_buffer_tracing_service) \
  MACRO(rocprofiler_create_buffer) \
  MACRO(rocprofiler_flush_buffer) \
  MACRO(rocprofiler_configure_callback_tracing_service) \
  MACRO(rocprofiler_push_external_correlation_id) \
  MACRO(rocprofiler_pop_external_correlation_id) \
  MACRO(rocprofiler_create_context) \
  MACRO(rocprofiler_start_context) \
  MACRO(rocprofiler_stop_context) \
  MACRO(rocprofiler_context_is_valid) \
  MACRO(rocprofiler_context_is_active) \
  MACRO(rocprofiler_query_record_counter_id) \
  MACRO(rocprofiler_query_record_dimension_position) \
  MACRO(rocprofiler_iterate_counter_dimensions) \
  MACRO(rocprofiler_query_counter_info) \
  MACRO(rocm_configure_counting_service) \
  MACRO(rocprofiler_configure_external_correlation_id_request_service) \
  MACRO(rocprofiler_configure_pc_sampling_service) \
  MACRO(rocprofiler_at_internal_thread_create) \
  MACRO(rocprofiler_create_callback_thread) \
  MACRO(rocprofiler_assign_callback_thread) \
  MACRO(rocprofiler_get_thread_id) \
  MACRO(rocprofiler_force_configure) \
  MACRO(rocprofiler_query_buffer_tracing_kind_name) \
  MACRO(rocprofiler_query_buffer_tracing_kind_operation_name) \
  MACRO(rocprofiler_get_status_string)



//******************************************************************************
// public variables
//******************************************************************************

const struct hpcrun_foil_appdispatch_rocm hpcrun_dispatch_rocm = {
#define INIT_FIELD(n) .n = &n,
  FORALL_ROCPROFILER_FUNCTIONS(INIT_FIELD)
#undef INIT_FIELD
};
