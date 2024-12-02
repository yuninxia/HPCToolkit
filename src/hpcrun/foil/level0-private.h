// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: BSD-3-Clause

#pragma once
#ifndef HPCRUN_FOIL_LEVEL0_PRIVATE_H
#define HPCRUN_FOIL_LEVEL0_PRIVATE_H

#ifdef __cplusplus
#error This is a C-only header
#endif

#include "level0.h"

struct hpcrun_foil_appdispatch_level0 {
  ze_result_t (*zeInit)(ze_init_flag_t);
  ze_result_t (*zeDriverGet)(uint32_t*, ze_driver_handle_t*);
  ze_result_t (*zeDeviceGet)(ze_driver_handle_t, uint32_t*, ze_device_handle_t*);
  ze_result_t (*zeDeviceGetProperties)(ze_device_handle_t, ze_device_properties_t*);
  ze_result_t (*zeEventCreate)(ze_event_pool_handle_t, const ze_event_desc_t*,
                               ze_event_handle_t*);
  ze_result_t (*zeEventDestroy)(ze_event_handle_t);
  ze_result_t (*zeEventPoolCreate)(ze_context_handle_t, const ze_event_pool_desc_t*,
                                   uint32_t, ze_device_handle_t*,
                                   ze_event_pool_handle_t*);
  ze_result_t (*zeEventPoolDestroy)(ze_event_pool_handle_t);
  ze_result_t (*zeEventQueryStatus)(ze_event_handle_t);
  ze_result_t (*zeEventQueryKernelTimestamp)(ze_event_handle_t,
                                             ze_kernel_timestamp_result_t*);
  ze_result_t (*zeMemGetAllocProperties)(ze_context_handle_t, const void* ptr,
                                         ze_memory_allocation_properties_t*,
                                         ze_device_handle_t*);
  ze_result_t (*zeCommandListAppendLaunchKernel)(
      ze_command_list_handle_t hCommandList, ze_kernel_handle_t hKernel,
      const ze_group_count_t* pLaunchFuncArgs, ze_event_handle_t hSignalEvent,
      uint32_t numWaitEvents, ze_event_handle_t* phWaitEvents);
  ze_result_t (*zeCommandListAppendMemoryCopy)(ze_command_list_handle_t hCommandList,
                                               void* dstptr, const void* srcptr,
                                               size_t size,
                                               ze_event_handle_t hSignalEvent,
                                               uint32_t numWaitEvents,
                                               ze_event_handle_t* phWaitEvents);
  ze_result_t (*zeCommandListCreate)(ze_context_handle_t hContext,
                                     ze_device_handle_t hDevice,
                                     const ze_command_list_desc_t* desc,
                                     ze_command_list_handle_t* phCommandList);
  ze_result_t (*zeCommandListCreateImmediate)(ze_context_handle_t hContext,
                                              ze_device_handle_t hDevice,
                                              const ze_command_queue_desc_t* desc,
                                              ze_command_list_handle_t* phCommandList);
  ze_result_t (*zeCommandListDestroy)(ze_command_list_handle_t hCommandList);
  ze_result_t (*zeCommandListReset)(ze_command_list_handle_t hCommandList);
  ze_result_t (*zeCommandQueueExecuteCommandLists)(
      ze_command_queue_handle_t hCommandQueue, uint32_t numCommandLists,
      ze_command_list_handle_t* phCommandLists, ze_fence_handle_t hFence);
  ze_result_t (*zeEventHostReset)(ze_event_handle_t hEvent);
  ze_result_t (*zeModuleCreate)(ze_context_handle_t hContext,
                                ze_device_handle_t hDevice,
                                const ze_module_desc_t* desc,
                                ze_module_handle_t* phModule,
                                ze_module_build_log_handle_t* phBuildLog);
  ze_result_t (*zeModuleDestroy)(ze_module_handle_t hModule);
  ze_result_t (*zeKernelCreate)(ze_module_handle_t hModule,
                                const ze_kernel_desc_t* desc,
                                ze_kernel_handle_t* phKernel);
  ze_result_t (*zeKernelDestroy)(ze_kernel_handle_t hKernel);
  ze_result_t (*zeFenceDestroy)(ze_fence_handle_t hFence);
  ze_result_t (*zeFenceReset)(ze_fence_handle_t hFence);
  ze_result_t (*zeCommandQueueSynchronize)(ze_command_queue_handle_t hCommandQueue,
                                           uint64_t timeout);
  ze_result_t (*zeKernelGetName)(ze_kernel_handle_t, size_t*, char*);
  ze_result_t (*zetModuleGetDebugInfo)(zet_module_handle_t,
                                       zet_module_debug_info_format_t, size_t*,
                                       uint8_t*);
  ze_result_t (*zetMetricGroupGetProperties)(zet_metric_group_handle_t,
                                             zet_metric_group_properties_t*);
  ze_result_t (*zeContextCreate)(ze_driver_handle_t, const ze_context_desc_t*, ze_context_handle_t*);
  ze_result_t (*zeDeviceGetSubDevices)(ze_device_handle_t, uint32_t*, ze_device_handle_t*);
  ze_result_t (*zeDeviceGetRootDevice)(ze_device_handle_t, ze_device_handle_t*);
  ze_result_t (*zeDriverGetApiVersion)(ze_driver_handle_t, ze_api_version_t*);
  ze_result_t (*zeEventHostSynchronize)(ze_event_handle_t, uint64_t);
  ze_result_t (*zeEventHostSignal)(ze_event_handle_t);
  ze_result_t (*zeModuleGetKernelNames)(ze_module_handle_t, uint32_t*, const char**);
  ze_result_t (*zeModuleGetFunctionPointer)(ze_module_handle_t, const char*, void**);
  ze_result_t (*zeKernelGetProperties)(ze_kernel_handle_t, ze_kernel_properties_t*);
  ze_result_t (*zeCommandListGetDeviceHandle)(ze_command_list_handle_t, ze_device_handle_t*);
  ze_result_t (*zetMetricGet)(zet_metric_group_handle_t, uint32_t*, zet_metric_handle_t*);
  ze_result_t (*zetMetricGetProperties)(zet_metric_handle_t, zet_metric_properties_t*);
  ze_result_t (*zetContextActivateMetricGroups)(zet_context_handle_t, ze_device_handle_t,
                                                uint32_t, zet_metric_group_handle_t*);
  ze_result_t (*zetMetricStreamerOpen)(zet_context_handle_t, ze_device_handle_t,
                                       zet_metric_group_handle_t, ze_event_handle_t,
                                       zet_metric_streamer_desc_t*, zet_metric_streamer_handle_t*);
  ze_result_t (*zetMetricStreamerClose)(zet_metric_streamer_handle_t);
  ze_result_t (*zetMetricGroupGet)(zet_device_handle_t, uint32_t*,
                                   zet_metric_group_handle_t*);
  ze_result_t (*zetMetricStreamerReadData)(zet_metric_streamer_handle_t,
                                           uint32_t, size_t*, uint8_t*);
  ze_result_t (*zetMetricGroupCalculateMultipleMetricValuesExp)(
                zet_metric_group_handle_t, zet_metric_group_calculation_type_t, size_t,
                const uint8_t*, uint32_t*, uint32_t*, uint32_t*, zet_typed_value_t*);
  ze_result_t (*zelTracerSetPrologues)(zel_tracer_handle_t, zel_core_callbacks_t*);
  ze_result_t (*zelTracerSetEpilogues)(zel_tracer_handle_t, zel_core_callbacks_t*);
  ze_result_t (*zelTracerSetEnabled)(zel_tracer_handle_t, ze_bool_t);
  ze_result_t (*zelTracerCreate)(const zel_tracer_desc_t*, zel_tracer_handle_t*);
  ze_result_t (*zelTracerDestroy)(zel_tracer_handle_t);
};

struct hpcrun_foil_hookdispatch_level0 {
  ze_result_t (*zeInit)(ze_init_flag_t, const struct hpcrun_foil_appdispatch_level0*);
  ze_result_t (*zeCommandListAppendLaunchKernel)(
      ze_command_list_handle_t, ze_kernel_handle_t, const ze_group_count_t*,
      ze_event_handle_t, uint32_t, ze_event_handle_t*,
      const struct hpcrun_foil_appdispatch_level0*);
  ze_result_t (*zeCommandListAppendMemoryCopy)(
      ze_command_list_handle_t, void*, const void*, size_t, ze_event_handle_t, uint32_t,
      ze_event_handle_t*, const struct hpcrun_foil_appdispatch_level0*);
  ze_result_t (*zeCommandListCreate)(ze_context_handle_t, ze_device_handle_t,
                                     const ze_command_list_desc_t*,
                                     ze_command_list_handle_t*,
                                     const struct hpcrun_foil_appdispatch_level0*);
  ze_result_t (*zeCommandListCreateImmediate)(
      ze_context_handle_t, ze_device_handle_t, const ze_command_queue_desc_t*,
      ze_command_list_handle_t*, const struct hpcrun_foil_appdispatch_level0*);
  ze_result_t (*zeCommandListDestroy)(ze_command_list_handle_t,
                                      const struct hpcrun_foil_appdispatch_level0*);
  ze_result_t (*zeCommandListReset)(ze_command_list_handle_t,
                                    const struct hpcrun_foil_appdispatch_level0*);
  ze_result_t (*zeCommandQueueExecuteCommandLists)(
      ze_command_queue_handle_t, uint32_t, ze_command_list_handle_t*, ze_fence_handle_t,
      const struct hpcrun_foil_appdispatch_level0*);
  ze_result_t (*zeEventPoolCreate)(ze_context_handle_t, const ze_event_pool_desc_t*,
                                   uint32_t, ze_device_handle_t*,
                                   ze_event_pool_handle_t*,
                                   const struct hpcrun_foil_appdispatch_level0*);
  ze_result_t (*zeEventDestroy)(ze_event_handle_t,
                                const struct hpcrun_foil_appdispatch_level0*);
  ze_result_t (*zeEventHostReset)(ze_event_handle_t,
                                  const struct hpcrun_foil_appdispatch_level0*);
  ze_result_t (*zeModuleCreate)(ze_context_handle_t, ze_device_handle_t,
                                const ze_module_desc_t*, ze_module_handle_t*,
                                ze_module_build_log_handle_t*,
                                const struct hpcrun_foil_appdispatch_level0*);
  ze_result_t (*zeModuleDestroy)(ze_module_handle_t,
                                 const struct hpcrun_foil_appdispatch_level0*);
  ze_result_t (*zeKernelCreate)(ze_module_handle_t, const ze_kernel_desc_t*,
                                ze_kernel_handle_t*,
                                const struct hpcrun_foil_appdispatch_level0*);
  ze_result_t (*zeKernelDestroy)(ze_kernel_handle_t,
                                 const struct hpcrun_foil_appdispatch_level0*);
  ze_result_t (*zeFenceDestroy)(ze_fence_handle_t,
                                const struct hpcrun_foil_appdispatch_level0*);
  ze_result_t (*zeFenceReset)(ze_fence_handle_t,
                              const struct hpcrun_foil_appdispatch_level0*);
  ze_result_t (*zeCommandQueueSynchronize)(ze_command_queue_handle_t, uint64_t,
                                           const struct hpcrun_foil_appdispatch_level0*);
};

#endif // HPCRUN_FOIL_LEVEL0_PRIVATE_H
