// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: Apache-2.0

//******************************************************************************
// system includes
//******************************************************************************

#define _GNU_SOURCE
#include <dlfcn.h>
#include <threads.h>

//******************************************************************************
// hpctoolkit includes
//******************************************************************************

#include "../gpu/api/rocm/rocm-configure.h"
#include "../hpcrun-sonames.h"
#include "common.h"
#include "rocm-private.h"
#include "rocm.h"

//******************************************************************************
// private variables
//******************************************************************************

static const struct hpcrun_foil_appdispatch_rocm* dispatch_var = NULL;

//******************************************************************************
// private operations
//******************************************************************************

static void init_dispatch(void) {
  void* handle = dlmopen(LM_ID_BASE, HPCRUN_DLOPEN_ROCM_SO, RTLD_NOW | RTLD_DEEPBIND);
  if (handle == NULL) {
    assert(false && "Failed to load foil_rocm.so");
    abort();
  }
  dispatch_var = dlsym(handle, "hpcrun_dispatch_rocm");
  if (dispatch_var == NULL) {
    assert(false && "Failed to fetch dispatch from foil_rocm.so");
    abort();
  }
}

static const struct hpcrun_foil_appdispatch_rocm* dispatch(void) {
  static once_flag once = ONCE_FLAG_INIT;
  call_once(&once, init_dispatch);
  return dispatch_var;
}

//******************************************************************************
// interface operations
//******************************************************************************

HPCRUN_EXPOSED_API const struct hpcrun_foil_hookdispatch_rocm*
hpcrun_foil_fetch_hooks_rocm(void) {
  static const struct hpcrun_foil_hookdispatch_rocm hooks = {
      .rocprofiler_configure = &foilbase_rocprofiler_configure,
  };
  return &hooks;
}

rocprofiler_status_t
f_rocprofiler_query_available_agents(rocprofiler_agent_version_t version,
                                     rocprofiler_query_available_agents_cb_t callback,
                                     size_t agent_size, void* user_data) {
  return dispatch()->rocprofiler_query_available_agents(version, callback, agent_size,
                                                        user_data);
}

rocprofiler_status_t f_rocprofiler_query_pc_sampling_agent_configurations(
    rocprofiler_agent_id_t agent_id,
    rocprofiler_available_pc_sampling_configurations_cb_t cb, void* user_data) {
  return dispatch()->rocprofiler_query_pc_sampling_agent_configurations(agent_id, cb,
                                                                        user_data);
}

rocprofiler_status_t
f_rocprofiler_iterate_agent_supported_counters(rocprofiler_agent_id_t agent_id,
                                               rocprofiler_available_counters_cb_t cb,
                                               void* user_data) {
  return dispatch()->rocprofiler_iterate_agent_supported_counters(agent_id, cb,
                                                                  user_data);
}

rocprofiler_status_t f_rocprofiler_create_profile_config(
    rocprofiler_agent_id_t agent_id, rocprofiler_counter_id_t* counters_list,
    size_t counters_count, rocprofiler_profile_config_id_t* config_id) {
  return dispatch()->rocprofiler_create_profile_config(agent_id, counters_list,
                                                       counters_count, config_id);
}

rocprofiler_status_t f_rocprofiler_configure_buffer_tracing_service(
    rocprofiler_context_id_t context_id, rocprofiler_buffer_tracing_kind_t kind,
    rocm_tracing_operation_t* operations, size_t operations_count,
    rocprofiler_buffer_id_t buffer_id) {
  return dispatch()->rocprofiler_configure_buffer_tracing_service(
      context_id, kind, operations, operations_count, buffer_id);
}

rocprofiler_status_t
f_rocprofiler_create_buffer(rocprofiler_context_id_t context, size_t size,
                            size_t watermark, rocprofiler_buffer_policy_t policy,
                            rocprofiler_buffer_tracing_cb_t callback,
                            void* callback_data, rocprofiler_buffer_id_t* buffer_id) {
  return dispatch()->rocprofiler_create_buffer(context, size, watermark, policy,
                                               callback, callback_data, buffer_id);
}

rocprofiler_status_t f_rocprofiler_flush_buffer(rocprofiler_buffer_id_t buffer_id) {
  return dispatch()->rocprofiler_flush_buffer(buffer_id);
}

rocprofiler_status_t f_rocprofiler_configure_callback_tracing_service(
    rocprofiler_context_id_t context_id, rocprofiler_callback_tracing_kind_t kind,
    rocprofiler_tracing_operation_t* operations, size_t operations_count,
    rocprofiler_callback_tracing_cb_t callback, void* callback_args) {
  return dispatch()->rocprofiler_configure_callback_tracing_service(
      context_id, kind, operations, operations_count, callback, callback_args);
}

rocprofiler_status_t f_rocprofiler_push_external_correlation_id(
    rocprofiler_context_id_t context, rocprofiler_thread_id_t tid,
    rocprofiler_user_data_t external_correlation_id) {
  return dispatch()->rocprofiler_push_external_correlation_id(context, tid,
                                                              external_correlation_id);
}

rocprofiler_status_t f_rocprofiler_pop_external_correlation_id(
    rocprofiler_context_id_t context, rocprofiler_thread_id_t tid,
    rocprofiler_user_data_t* external_correlation_id) {
  return dispatch()->rocprofiler_pop_external_correlation_id(context, tid,
                                                             external_correlation_id);
}

rocprofiler_status_t
f_rocprofiler_create_context(rocprofiler_context_id_t* context_id) {
  return dispatch()->rocprofiler_create_context(context_id);
}

rocprofiler_status_t f_rocprofiler_start_context(rocprofiler_context_id_t context_id) {
  return dispatch()->rocprofiler_start_context(context_id);
}

rocprofiler_status_t f_rocprofiler_stop_context(rocprofiler_context_id_t context_id) {
  return dispatch()->rocprofiler_stop_context(context_id);
}

rocprofiler_status_t f_rocprofiler_context_is_valid(rocprofiler_context_id_t context_id,
                                                    int* status) {
  return dispatch()->rocprofiler_context_is_valid(context_id, status);
}

rocprofiler_status_t
f_rocprofiler_context_is_active(rocprofiler_context_id_t context_id, int* status) {
  return dispatch()->rocprofiler_context_is_active(context_id, status);
}

rocprofiler_status_t
f_rocprofiler_query_record_counter_id(rocprofiler_counter_instance_id_t id,
                                      rocprofiler_counter_id_t* counter_id) {
  return dispatch()->rocprofiler_query_record_counter_id(id, counter_id);
}

rocprofiler_status_t
f_rocprofiler_query_record_dimension_position(rocprofiler_counter_instance_id_t id,
                                              rocprofiler_counter_dimension_id_t dim,
                                              size_t* pos) {
  return dispatch()->rocprofiler_query_record_dimension_position(id, dim, pos);
}

rocprofiler_status_t
f_rocprofiler_iterate_counter_dimensions(rocprofiler_counter_id_t id,
                                         rocprofiler_available_dimensions_cb_t info_cb,
                                         void* user_data) {
  return dispatch()->rocprofiler_iterate_counter_dimensions(id, info_cb, user_data);
}

rocprofiler_status_t
f_rocprofiler_query_counter_info(rocprofiler_counter_id_t counter_id,
                                 rocprofiler_counter_info_version_id_t version,
                                 void* info) {
  return dispatch()->rocprofiler_query_counter_info(counter_id, version, info);
}

rocprofiler_status_t f_rocm_configure_counting_service(
    rocprofiler_context_id_t context_id, rocprofiler_buffer_id_t buffer_id,
    rocm_counting_service_callback_t callback, void* callback_data_args) {
  return dispatch()->rocm_configure_counting_service(context_id, buffer_id, callback,
                                                     callback_data_args);
}

rocprofiler_status_t f_rocprofiler_configure_external_correlation_id_request_service(
    rocprofiler_context_id_t context_id,
    rocm_external_correlation_id_request_kind_t* kinds, size_t kinds_count,
    rocprofiler_external_correlation_id_request_cb_t callback, void* callback_args) {
  return dispatch()->rocprofiler_configure_external_correlation_id_request_service(
      context_id, kinds, kinds_count, callback, callback_args);
}

rocprofiler_status_t f_rocprofiler_configure_pc_sampling_service(
    rocprofiler_context_id_t context_id, rocprofiler_agent_id_t agent_id,
    rocprofiler_pc_sampling_method_t method, rocprofiler_pc_sampling_unit_t unit,
    uint64_t interval,
    rocprofiler_buffer_id_t buffer_id CONFIGURE_PC_SAMPLING_SERVICE_FLAG_DECL) {
  return dispatch()->rocprofiler_configure_pc_sampling_service(
      context_id, agent_id, method, unit, interval,
      buffer_id CONFIGURE_PC_SAMPLING_SERVICE_FLAG_ARG);
}

rocprofiler_status_t f_rocprofiler_at_internal_thread_create(
    rocprofiler_internal_thread_library_cb_t precreate,
    rocprofiler_internal_thread_library_cb_t postcreate, int libs, void* data) {
  return dispatch()->rocprofiler_at_internal_thread_create(precreate, postcreate, libs,
                                                           data);
}

rocprofiler_status_t
f_rocprofiler_create_callback_thread(rocprofiler_callback_thread_t* cb_thread_id) {
  return dispatch()->rocprofiler_create_callback_thread(cb_thread_id);
}

rocprofiler_status_t
f_rocprofiler_assign_callback_thread(rocprofiler_buffer_id_t buffer_id,
                                     rocprofiler_callback_thread_t cb_thread_id) {
  return dispatch()->rocprofiler_assign_callback_thread(buffer_id, cb_thread_id);
}

rocprofiler_status_t f_rocprofiler_get_thread_id(rocprofiler_thread_id_t* tid) {
  return dispatch()->rocprofiler_get_thread_id(tid);
}

rocprofiler_status_t
f_rocprofiler_force_configure(rocprofiler_configure_func_t configure_func) {
  return dispatch()->rocprofiler_force_configure(configure_func);
}

rocprofiler_status_t
f_rocprofiler_query_buffer_tracing_kind_name(rocprofiler_buffer_tracing_kind_t kind,
                                             const char** name, uint64_t* name_len) {
  return dispatch()->rocprofiler_query_buffer_tracing_kind_name(kind, name, name_len);
}

rocprofiler_status_t f_rocprofiler_query_buffer_tracing_kind_operation_name(
    rocprofiler_buffer_tracing_kind_t kind, rocprofiler_tracing_operation_t operation,
    const char** name, uint64_t* name_len) {
  return dispatch()->rocprofiler_query_buffer_tracing_kind_operation_name(
      kind, operation, name, name_len);
}

const char* f_rocprofiler_get_status_string(rocprofiler_status_t status) {
  return dispatch()->rocprofiler_get_status_string(status);
}
