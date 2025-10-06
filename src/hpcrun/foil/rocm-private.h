// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: Apache-2.0

#pragma once
#ifndef HPCRUN_FOIL_ROCM_PRIVATE_H
#define HPCRUN_FOIL_ROCM_PRIVATE_H

#ifdef __cplusplus
#error This is a C-only header
#endif

//******************************************************************************
// hpctoolkit includes
//******************************************************************************

#include "common.h"

// import abstractions for rocprofiler changes
#include "../gpu/api/rocm/rocprofiler.h"
#include "rocm.h"

//******************************************************************************
// type declarations
//******************************************************************************

struct hpcrun_foil_hookdispatch_rocm {
  rocprofiler_tool_configure_result_t* (*rocprofiler_configure)(
      uint32_t version, const char* runtime_version, uint32_t priority,
      rocprofiler_client_id_t* id);
};

struct hpcrun_foil_appdispatch_rocm {

  rocprofiler_status_t (*rocprofiler_query_available_agents)(
      rocprofiler_agent_version_t version,
      rocprofiler_query_available_agents_cb_t callback, size_t agent_size,
      void* user_data);

  rocprofiler_status_t (*rocprofiler_query_pc_sampling_agent_configurations)(
      rocprofiler_agent_id_t agent_id,
      rocprofiler_available_pc_sampling_configurations_cb_t cb, void* user_data);

  rocprofiler_status_t (*rocprofiler_iterate_agent_supported_counters)(
      rocprofiler_agent_id_t agent_id, rocprofiler_available_counters_cb_t cb,
      void* user_data);

  rocprofiler_status_t (*rocprofiler_create_profile_config)(
      rocprofiler_agent_id_t agent_id, rocprofiler_counter_id_t* counters_list,
      size_t counters_count, rocprofiler_profile_config_id_t* config_id);

  rocprofiler_status_t (*rocprofiler_configure_buffer_tracing_service)(
      rocprofiler_context_id_t context_id, rocprofiler_buffer_tracing_kind_t kind,
      rocm_tracing_operation_t* operations, size_t operations_count,
      rocprofiler_buffer_id_t buffer_id);

  rocprofiler_status_t (*rocprofiler_create_buffer)(
      rocprofiler_context_id_t context, size_t size, size_t watermark,
      rocprofiler_buffer_policy_t policy, rocprofiler_buffer_tracing_cb_t callback,
      void* callback_data, rocprofiler_buffer_id_t* buffer_id);

  rocprofiler_status_t (*rocprofiler_flush_buffer)(rocprofiler_buffer_id_t buffer_id);

  rocprofiler_status_t (*rocprofiler_configure_callback_tracing_service)(
      rocprofiler_context_id_t context_id, rocprofiler_callback_tracing_kind_t kind,
      rocm_tracing_operation_t* operations, size_t operations_count,
      rocprofiler_callback_tracing_cb_t callback, void* callback_args);

  rocprofiler_status_t (*rocprofiler_push_external_correlation_id)(
      rocprofiler_context_id_t context, rocprofiler_thread_id_t tid,
      rocprofiler_user_data_t external_correlation_id);

  rocprofiler_status_t (*rocprofiler_pop_external_correlation_id)(
      rocprofiler_context_id_t context, rocprofiler_thread_id_t tid,
      rocprofiler_user_data_t* external_correlation_id);

  rocprofiler_status_t (*rocprofiler_create_context)(
      rocprofiler_context_id_t* context_id);

  rocprofiler_status_t (*rocprofiler_start_context)(rocprofiler_context_id_t context_id);

  rocprofiler_status_t (*rocprofiler_stop_context)(rocprofiler_context_id_t context_id);

  rocprofiler_status_t (*rocprofiler_context_is_valid)(
      rocprofiler_context_id_t context_id, int* status);

  rocprofiler_status_t (*rocprofiler_context_is_active)(
      rocprofiler_context_id_t context_id, int* status);

  rocprofiler_status_t (*rocprofiler_query_record_counter_id)(
      rocprofiler_counter_instance_id_t id, rocprofiler_counter_id_t* counter_id);

  rocprofiler_status_t (*rocprofiler_query_record_dimension_position)(
      rocprofiler_counter_instance_id_t id, rocprofiler_counter_dimension_id_t dim,
      size_t* pos);

  rocprofiler_status_t (*rocprofiler_iterate_counter_dimensions)(
      rocprofiler_counter_id_t id, rocprofiler_available_dimensions_cb_t info_cb,
      void* user_data);

  rocprofiler_status_t (*rocprofiler_query_counter_info)(
      rocprofiler_counter_id_t counter_id,
      rocprofiler_counter_info_version_id_t version, void* info);

  rocprofiler_status_t (*rocm_configure_counting_service)(
      rocprofiler_context_id_t context_id, rocprofiler_buffer_id_t buffer_id,
      rocm_counting_service_callback_t callback, void* callback_data_args);

  rocprofiler_status_t (*rocprofiler_configure_external_correlation_id_request_service)(
      rocprofiler_context_id_t context_id,
      rocm_external_correlation_id_request_kind_t* kinds, size_t kinds_count,
      rocprofiler_external_correlation_id_request_cb_t callback, void* callback_args);

  rocprofiler_status_t (*rocprofiler_configure_pc_sampling_service)(
      rocprofiler_context_id_t context_id, rocprofiler_agent_id_t agent_id,
      rocprofiler_pc_sampling_method_t method, rocprofiler_pc_sampling_unit_t unit,
      uint64_t interval,
      rocprofiler_buffer_id_t buffer_id CONFIGURE_PC_SAMPLING_SERVICE_FLAG_DECL);

  rocprofiler_status_t (*rocprofiler_at_internal_thread_create)(
      rocprofiler_internal_thread_library_cb_t precreate,
      rocprofiler_internal_thread_library_cb_t postcreate, int libs, void* data);

  rocprofiler_status_t (*rocprofiler_create_callback_thread)(
      rocprofiler_callback_thread_t* cb_thread_id);

  rocprofiler_status_t (*rocprofiler_assign_callback_thread)(
      rocprofiler_buffer_id_t buffer_id, rocprofiler_callback_thread_t cb_thread_id);

  rocprofiler_status_t (*rocprofiler_get_thread_id)(rocprofiler_thread_id_t* tid);

  rocprofiler_status_t (*rocprofiler_force_configure)(
      rocprofiler_configure_func_t configure_func);

  rocprofiler_status_t (*rocprofiler_query_buffer_tracing_kind_name)(
      rocprofiler_buffer_tracing_kind_t kind, const char** name, uint64_t* name_len);

  rocprofiler_status_t (*rocprofiler_query_buffer_tracing_kind_operation_name)(
      rocprofiler_buffer_tracing_kind_t kind, rocprofiler_tracing_operation_t operation,
      const char** name, uint64_t* name_len);

  const char* (*rocprofiler_get_status_string)(rocprofiler_status_t status);
};

//******************************************************************************
// public variables
//******************************************************************************

HPCRUN_EXPOSED_API const struct hpcrun_foil_appdispatch_rocm hpcrun_dispatch_rocm;

#endif // HPCRUN_FOIL_ROCM_PRIVATE_H
