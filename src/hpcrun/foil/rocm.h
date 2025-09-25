// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: Apache-2.0

#pragma once
#ifndef HPCRUN_FOIL_ROCM_H
#define HPCRUN_FOIL_ROCM_H

//******************************************************************************
// rocprofiler includes
//******************************************************************************

#include "../gpu/api/rocm/rocprofiler.h"

#if (ROCPROFILER_VERSION_MAJOR == 0 && ROCPROFILER_VERSION_MINOR == 4)

#define rocm_configure_counting_service                                                \
  rocprofiler_configure_buffered_dispatch_profile_counting_service

#define rocm_counting_service_callback_t                                               \
  rocprofiler_profile_counting_dispatch_callback_t

#define rocm_external_correlation_id_request_kind_t                                    \
  rocprofiler_external_correlation_id_request_kind_t

#define rocm_tracing_operation_t rocprofiler_tracing_operation_t

typedef uint64_t rocprofiler_pc_t;

#define rocprofiler_dispatch_counting_service_data_t                                   \
  rocprofiler_profile_counting_dispatch_data_t

#define rocprofiler_dispatch_counting_service_record_t                                 \
  rocprofiler_profile_counting_dispatch_record_t

#define ROCPROFILER_PC_SAMPLING_RECORD_CODE_OBJECT_LOAD_MARKER                         \
  ROCPROFILER_PC_SAMPLING_RECORD_CODE_OBJECT_LOAD_MARKER
#define ROCPROFILER_PC_SAMPLING_RECORD_CODE_OBJECT_UNLOAD_MARKER                       \
  ROCPROFILER_PC_SAMPLING_RECORD_CODE_OBJECT_UNLOAD_MARKER

#else

#define rocm_configure_counting_service                                                \
  rocprofiler_configure_buffered_dispatch_counting_service

#define rocm_counting_service_callback_t                                               \
  rocprofiler_dispatch_counting_service_callback_t

#define rocm_external_correlation_id_request_kind_t                                    \
  const rocprofiler_external_correlation_id_request_kind_t

#define rocm_tracing_operation_t const rocprofiler_tracing_operation_t

#endif

#define rocprofiler_async_correlation_id_t rocprofiler_correlation_id_t

#if (ROCPROFILER_VERSION_MAJOR >= 1)

#undef rocprofiler_async_correlation_id_t

#undef rocm_configure_counting_service
#define rocm_configure_counting_service                                                \
  rocprofiler_configure_buffer_dispatch_counting_service

#undef rocm_counting_service_callback_t
#define rocm_counting_service_callback_t rocprofiler_dispatch_counting_service_cb_t

#define rocprofiler_profile_config_id_t rocprofiler_counter_config_id_t

#define rocprofiler_destroy_profile_config rocprofiler_destroy_counter_config

#define rocprofiler_create_profile_config rocprofiler_create_counter_config

#endif

// PC sampling record kinds
#if (ROCPROFILER_VERSION_MAJOR == 0 && ROCPROFILER_VERSION_MINOR < 6)
#define ROCPROFILER_PC_SAMPLING_RECORD_HOST ROCPROFILER_PC_SAMPLING_RECORD_SAMPLE
#define wave_in_group wave_id                    // rocm 6.4 changed the field name
#define CONFIGURE_PC_SAMPLING_SERVICE_FLAG_PARAM // none
#define CONFIGURE_PC_SAMPLING_SERVICE_FLAG_ARG   // none
#define CONFIGURE_PC_SAMPLING_SERVICE_FLAG_DECL  // none
#define CONFIGURE_PC_SAMPLING_SERVICE_FLAG_VALUE // none
#else
#define ROCPROFILER_PC_SAMPLING_RECORD_HOST                                            \
  ROCPROFILER_PC_SAMPLING_RECORD_HOST_TRAP_V0_SAMPLE
#define ROCPROFILER_PC_SAMPLING_RECORD_STOCHASTIC                                      \
  ROCPROFILER_PC_SAMPLING_RECORD_STOCHASTIC_V0_SAMPLE
#define rocprofiler_pc_sampling_record_sw_t rocprofiler_pc_sampling_record_host_trap_v0_t
#define rocprofiler_pc_sampling_record_hw_t rocprofiler_pc_sampling_record_stochastic_v0_t
#define CONFIGURE_PC_SAMPLING_SERVICE_FLAG_PARAM flags
#define CONFIGURE_PC_SAMPLING_SERVICE_FLAG_ARG                                         \
  , CONFIGURE_PC_SAMPLING_SERVICE_FLAG_PARAM
#define CONFIGURE_PC_SAMPLING_SERVICE_FLAG_DECL                                        \
  , int CONFIGURE_PC_SAMPLING_SERVICE_FLAG_PARAM
#define CONFIGURE_PC_SAMPLING_SERVICE_FLAG_VALUE , 0
#endif

#if (ROCPROFILER_VERSION_MAJOR == 0 && ROCPROFILER_VERSION_MINOR == 4)
#define PC_FORMAT "(%p)"
#define PC_VALUE(pc) (void*)(pc)
#else
#define PC_FORMAT "pc=(co_id=%lu, offset=0x%lx)"
#define PC_VALUE(pc) pc.loaded_code_object_id, pc.loaded_code_object_offset
#endif

#if (ROCPROFILER_VERSION_MAJOR == 0 && ROCPROFILER_VERSION_MINOR > 5)
#define ROCM_AGENT_VISIBLE(agent) (agent)->runtime_visibility.hsa
#define ROCPROFILER_BUFFER_TRACING_MEMORY_ALLOCATION                                   \
  ROCPROFILER_BUFFER_TRACING_MEMORY_ALLOCATION
#else
#define ROCM_AGENT_VISIBLE(agent) 1
#endif

#if ((ROCPROFILER_VERSION_MAJOR == 0 && ROCPROFILER_VERSION_MINOR > 5) ||              \
     (ROCPROFILER_VERSION_MAJOR == 1))
#define loaded_code_object_id code_object_id
#define loaded_code_object_offset code_object_offset
#define ROCPROFILER_OMPT_AVAILABLE
// #define ROCPROFILER_PAGE_MIGRATION_AVAILABLE
#endif

//******************************************************************************
// interface operations
//******************************************************************************

#ifdef __cplusplus
extern "C" {
#endif

rocprofiler_status_t
f_rocprofiler_query_available_agents(rocprofiler_agent_version_t version,
                                     rocprofiler_query_available_agents_cb_t callback,
                                     size_t agent_size, void* user_data);

rocprofiler_status_t f_rocprofiler_query_pc_sampling_agent_configurations(
    rocprofiler_agent_id_t agent_id,
    rocprofiler_available_pc_sampling_configurations_cb_t cb, void* user_data);

rocprofiler_status_t
f_rocprofiler_iterate_agent_supported_counters(rocprofiler_agent_id_t agent_id,
                                               rocprofiler_available_counters_cb_t cb,
                                               void* user_data);

rocprofiler_status_t f_rocprofiler_create_profile_config(
    rocprofiler_agent_id_t agent_id, rocprofiler_counter_id_t* counters_list,
    size_t counters_count, rocprofiler_profile_config_id_t* config_id);

rocprofiler_status_t f_rocprofiler_configure_buffer_tracing_service(
    rocprofiler_context_id_t context_id, rocprofiler_buffer_tracing_kind_t kind,
    rocm_tracing_operation_t* operations, size_t operations_count,
    rocprofiler_buffer_id_t buffer_id);

rocprofiler_status_t ROCPROFILER_API f_rocprofiler_create_buffer(
    rocprofiler_context_id_t context, size_t size, size_t watermark,
    rocprofiler_buffer_policy_t policy, rocprofiler_buffer_tracing_cb_t callback,
    void* callback_data, rocprofiler_buffer_id_t* buffer_id);

rocprofiler_status_t f_rocprofiler_flush_buffer(rocprofiler_buffer_id_t buffer_id);

rocprofiler_status_t f_rocprofiler_configure_callback_tracing_service(
    rocprofiler_context_id_t context_id, rocprofiler_callback_tracing_kind_t kind,
    rocprofiler_tracing_operation_t* operations, size_t operations_count,
    rocprofiler_callback_tracing_cb_t callback, void* callback_args);

rocprofiler_status_t f_rocprofiler_push_external_correlation_id(
    rocprofiler_context_id_t context, rocprofiler_thread_id_t tid,
    rocprofiler_user_data_t external_correlation_id);

rocprofiler_status_t f_rocprofiler_pop_external_correlation_id(
    rocprofiler_context_id_t context, rocprofiler_thread_id_t tid,
    rocprofiler_user_data_t* external_correlation_id);

rocprofiler_status_t ROCPROFILER_API
f_rocprofiler_create_context(rocprofiler_context_id_t* context_id);

rocprofiler_status_t f_rocprofiler_start_context(rocprofiler_context_id_t context_id);

rocprofiler_status_t f_rocprofiler_stop_context(rocprofiler_context_id_t context_id);

rocprofiler_status_t ROCPROFILER_API
f_rocprofiler_context_is_valid(rocprofiler_context_id_t context_id, int* status);

rocprofiler_status_t
f_rocprofiler_context_is_active(rocprofiler_context_id_t context_id, int* status);

rocprofiler_status_t ROCPROFILER_API f_rocprofiler_query_record_counter_id(
    rocprofiler_counter_instance_id_t id, rocprofiler_counter_id_t* counter_id);

rocprofiler_status_t
f_rocprofiler_query_record_dimension_position(rocprofiler_counter_instance_id_t id,
                                              rocprofiler_counter_dimension_id_t dim,
                                              size_t* pos);

rocprofiler_status_t ROCPROFILER_API f_rocprofiler_iterate_counter_dimensions(
    rocprofiler_counter_id_t id, rocprofiler_available_dimensions_cb_t info_cb,
    void* user_data);

rocprofiler_status_t ROCPROFILER_API f_rocprofiler_query_counter_info(
    rocprofiler_counter_id_t counter_id, rocprofiler_counter_info_version_id_t version,
    void* info);

rocprofiler_status_t f_rocm_configure_counting_service(
    rocprofiler_context_id_t context_id, rocprofiler_buffer_id_t buffer_id,
    rocm_counting_service_callback_t callback, void* callback_data_args);

rocprofiler_status_t f_rocprofiler_configure_external_correlation_id_request_service(
    rocprofiler_context_id_t context_id,
    rocm_external_correlation_id_request_kind_t* kinds, size_t kinds_count,
    rocprofiler_external_correlation_id_request_cb_t callback, void* callback_args);

rocprofiler_status_t f_rocprofiler_configure_pc_sampling_service(
    rocprofiler_context_id_t context_id, rocprofiler_agent_id_t agent_id,
    rocprofiler_pc_sampling_method_t method, rocprofiler_pc_sampling_unit_t unit,
    uint64_t interval,
    rocprofiler_buffer_id_t buffer_id CONFIGURE_PC_SAMPLING_SERVICE_FLAG_DECL);

rocprofiler_status_t f_rocprofiler_at_internal_thread_create(
    rocprofiler_internal_thread_library_cb_t precreate,
    rocprofiler_internal_thread_library_cb_t postcreate, int libs, void* data);

rocprofiler_status_t
f_rocprofiler_create_callback_thread(rocprofiler_callback_thread_t* cb_thread_id);

rocprofiler_status_t
f_rocprofiler_assign_callback_thread(rocprofiler_buffer_id_t buffer_id,
                                     rocprofiler_callback_thread_t cb_thread_id);

rocprofiler_status_t f_rocprofiler_get_thread_id(rocprofiler_thread_id_t* tid);

rocprofiler_status_t
f_rocprofiler_force_configure(rocprofiler_configure_func_t configure_func);

rocprofiler_status_t
f_rocprofiler_query_buffer_tracing_kind_name(rocprofiler_buffer_tracing_kind_t kind,
                                             const char** name, uint64_t* name_len);

rocprofiler_status_t f_rocprofiler_query_buffer_tracing_kind_operation_name(
    rocprofiler_buffer_tracing_kind_t kind, rocprofiler_tracing_operation_t operation,
    const char** name, uint64_t* name_len);

const char* f_rocprofiler_get_status_string(rocprofiler_status_t status);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // HPCRUN_FOIL_ROCM_H
