// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: BSD-3-Clause

#pragma once
#ifndef HPCRUN_FOIL_LEVEL0_H
#define HPCRUN_FOIL_LEVEL0_H

#include <level_zero/layers/zel_tracing_api.h>
#include <ze_api.h>
#include <zet_api.h>

#ifdef __cplusplus
extern "C" {
#endif

struct hpcrun_foil_appdispatch_level0;

ze_result_t f_zeInit(ze_init_flag_t, const struct hpcrun_foil_appdispatch_level0*);
ze_result_t f_zeDriverGet(uint32_t*, ze_driver_handle_t*,
                          const struct hpcrun_foil_appdispatch_level0*);
ze_result_t f_zeDeviceGet(ze_driver_handle_t, uint32_t*, ze_device_handle_t*,
                          const struct hpcrun_foil_appdispatch_level0*);
ze_result_t f_zeDeviceGetProperties(ze_device_handle_t, ze_device_properties_t*,
                                    const struct hpcrun_foil_appdispatch_level0*);
ze_result_t f_zeEventCreate(ze_event_pool_handle_t, const ze_event_desc_t*,
                            ze_event_handle_t*,
                            const struct hpcrun_foil_appdispatch_level0*);
ze_result_t f_zeEventDestroy(ze_event_handle_t,
                             const struct hpcrun_foil_appdispatch_level0*);
ze_result_t f_zeEventPoolCreate(ze_context_handle_t, const ze_event_pool_desc_t*,
                                uint32_t, ze_device_handle_t*, ze_event_pool_handle_t*,
                                const struct hpcrun_foil_appdispatch_level0*);
ze_result_t f_zeEventPoolDestroy(ze_event_pool_handle_t,
                                 const struct hpcrun_foil_appdispatch_level0*);
ze_result_t f_zeEventQueryStatus(ze_event_handle_t,
                                 const struct hpcrun_foil_appdispatch_level0*);
ze_result_t f_zeEventQueryKernelTimestamp(ze_event_handle_t,
                                          ze_kernel_timestamp_result_t*,
                                          const struct hpcrun_foil_appdispatch_level0*);
ze_result_t f_zeMemGetAllocProperties(ze_context_handle_t, const void* ptr,
                                      ze_memory_allocation_properties_t*,
                                      ze_device_handle_t*,
                                      const struct hpcrun_foil_appdispatch_level0*);
ze_result_t f_zeCommandListAppendLaunchKernel(
    ze_command_list_handle_t hCommandList, ze_kernel_handle_t hKernel,
    const ze_group_count_t* pLaunchFuncArgs, ze_event_handle_t hSignalEvent,
    uint32_t numWaitEvents, ze_event_handle_t* phWaitEvents,
    const struct hpcrun_foil_appdispatch_level0*);
ze_result_t f_zeCommandListAppendMemoryCopy(
    ze_command_list_handle_t hCommandList, void* dstptr, const void* srcptr,
    size_t size, ze_event_handle_t hSignalEvent, uint32_t numWaitEvents,
    ze_event_handle_t* phWaitEvents, const struct hpcrun_foil_appdispatch_level0*);
ze_result_t f_zeCommandListCreate(ze_context_handle_t hContext,
                                  ze_device_handle_t hDevice,
                                  const ze_command_list_desc_t* desc,
                                  ze_command_list_handle_t* phCommandList,
                                  const struct hpcrun_foil_appdispatch_level0*);
ze_result_t
f_zeCommandListCreateImmediate(ze_context_handle_t hContext, ze_device_handle_t hDevice,
                               const ze_command_queue_desc_t* desc,
                               ze_command_list_handle_t* phCommandList,
                               const struct hpcrun_foil_appdispatch_level0*);
ze_result_t f_zeCommandListDestroy(ze_command_list_handle_t hCommandList,
                                   const struct hpcrun_foil_appdispatch_level0*);
ze_result_t f_zeCommandListReset(ze_command_list_handle_t hCommandList,
                                 const struct hpcrun_foil_appdispatch_level0*);
ze_result_t f_zeCommandQueueExecuteCommandLists(
    ze_command_queue_handle_t hCommandQueue, uint32_t numCommandLists,
    ze_command_list_handle_t* phCommandLists, ze_fence_handle_t hFence,
    const struct hpcrun_foil_appdispatch_level0*);
ze_result_t f_zeEventHostReset(ze_event_handle_t hEvent,
                               const struct hpcrun_foil_appdispatch_level0*);
ze_result_t f_zeModuleCreate(ze_context_handle_t hContext, ze_device_handle_t hDevice,
                             const ze_module_desc_t* desc, ze_module_handle_t* phModule,
                             ze_module_build_log_handle_t* phBuildLog,
                             const struct hpcrun_foil_appdispatch_level0*);
ze_result_t f_zeModuleDestroy(ze_module_handle_t hModule,
                              const struct hpcrun_foil_appdispatch_level0*);
ze_result_t f_zeKernelCreate(ze_module_handle_t hModule, const ze_kernel_desc_t* desc,
                             ze_kernel_handle_t* phKernel,
                             const struct hpcrun_foil_appdispatch_level0*);
ze_result_t f_zeKernelDestroy(ze_kernel_handle_t hKernel,
                              const struct hpcrun_foil_appdispatch_level0*);
ze_result_t f_zeFenceDestroy(ze_fence_handle_t hFence,
                             const struct hpcrun_foil_appdispatch_level0*);
ze_result_t f_zeFenceReset(ze_fence_handle_t hFence,
                           const struct hpcrun_foil_appdispatch_level0*);
ze_result_t f_zeCommandQueueSynchronize(ze_command_queue_handle_t hCommandQueue,
                                        uint64_t timeout,
                                        const struct hpcrun_foil_appdispatch_level0*);
ze_result_t f_zeKernelGetName(ze_kernel_handle_t, size_t*, char*,
                              const struct hpcrun_foil_appdispatch_level0*);
ze_result_t f_zetModuleGetDebugInfo(zet_module_handle_t, zet_module_debug_info_format_t,
                                    size_t*, uint8_t*,
                                    const struct hpcrun_foil_appdispatch_level0*);
ze_result_t f_zetMetricGroupGetProperties(zet_metric_group_handle_t,
                                          zet_metric_group_properties_t*,
                                          const struct hpcrun_foil_appdispatch_level0*);
ze_result_t f_zeContextCreate(ze_driver_handle_t hDriver, const ze_context_desc_t* desc,
                              ze_context_handle_t* phContext,
                              const struct hpcrun_foil_appdispatch_level0*);
ze_result_t f_zeDeviceGetSubDevices(ze_device_handle_t, uint32_t*, ze_device_handle_t*,
                                    const struct hpcrun_foil_appdispatch_level0*);
ze_result_t f_zeDeviceGetRootDevice(ze_device_handle_t, ze_device_handle_t*,
                                    const struct hpcrun_foil_appdispatch_level0*);
ze_result_t f_zeDriverGetApiVersion(ze_driver_handle_t, ze_api_version_t*,
                                    const struct hpcrun_foil_appdispatch_level0*);
ze_result_t f_zeEventHostSynchronize(ze_event_handle_t, uint64_t,
                                    const struct hpcrun_foil_appdispatch_level0*);
ze_result_t f_zeEventHostSignal(ze_event_handle_t,
                                const struct hpcrun_foil_appdispatch_level0*);
ze_result_t f_zeModuleGetKernelNames(ze_module_handle_t, uint32_t*, const char**,
                                     const struct hpcrun_foil_appdispatch_level0*);
ze_result_t f_zeModuleGetFunctionPointer(ze_module_handle_t, const char*, void**,
                                         const struct hpcrun_foil_appdispatch_level0*);
ze_result_t f_zeKernelGetProperties(ze_kernel_handle_t, ze_kernel_properties_t*,
                                    const struct hpcrun_foil_appdispatch_level0*);
ze_result_t f_zeCommandListGetDeviceHandle(ze_command_list_handle_t, ze_device_handle_t*,
                                           const struct hpcrun_foil_appdispatch_level0*);
ze_result_t f_zetMetricGet(zet_metric_group_handle_t,
                           uint32_t*, zet_metric_handle_t*,
                           const struct hpcrun_foil_appdispatch_level0*);
ze_result_t f_zetMetricGetProperties(zet_metric_handle_t,
                                     zet_metric_properties_t*,
                                     const struct hpcrun_foil_appdispatch_level0*);
ze_result_t f_zetContextActivateMetricGroups(zet_context_handle_t, ze_device_handle_t,
                                             uint32_t, zet_metric_group_handle_t*,
                                             const struct hpcrun_foil_appdispatch_level0*);
ze_result_t f_zetMetricStreamerOpen(zet_context_handle_t, ze_device_handle_t,
                                    zet_metric_group_handle_t, zet_metric_streamer_desc_t*,
                                    ze_event_handle_t, zet_metric_streamer_handle_t*,
                                    const struct hpcrun_foil_appdispatch_level0*);
ze_result_t f_zetMetricStreamerClose(zet_metric_streamer_handle_t,
                                     const struct hpcrun_foil_appdispatch_level0*);
ze_result_t f_zetMetricGroupGet(zet_device_handle_t, uint32_t*,
                                zet_metric_group_handle_t*,
                                const struct hpcrun_foil_appdispatch_level0*);
ze_result_t f_zetMetricStreamerReadData(zet_metric_streamer_handle_t,
                                        uint32_t, size_t*, uint8_t*,
                                        const struct hpcrun_foil_appdispatch_level0*);
ze_result_t f_zetMetricGroupCalculateMultipleMetricValuesExp(zet_metric_group_handle_t,
                zet_metric_group_calculation_type_t, size_t, const uint8_t*, uint32_t*,
                uint32_t*, uint32_t*, zet_typed_value_t*, const struct hpcrun_foil_appdispatch_level0*);
ze_result_t f_zelTracerSetPrologues(zel_tracer_handle_t, zel_core_callbacks_t*,
                                    const struct hpcrun_foil_appdispatch_level0*);
ze_result_t f_zelTracerSetEpilogues(zel_tracer_handle_t, zel_core_callbacks_t*,
                                    const struct hpcrun_foil_appdispatch_level0*);
ze_result_t f_zelTracerSetEnabled(zel_tracer_handle_t, ze_bool_t,
                                  const struct hpcrun_foil_appdispatch_level0*);
ze_result_t f_zelTracerCreate(const zel_tracer_desc_t*, zel_tracer_handle_t*,
                              const struct hpcrun_foil_appdispatch_level0*);
ze_result_t f_zelTracerDestroy(zel_tracer_handle_t, const struct hpcrun_foil_appdispatch_level0*);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // HPCRUN_FOIL_LEVEL0_H
