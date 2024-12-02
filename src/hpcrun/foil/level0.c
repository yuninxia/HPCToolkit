// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: BSD-3-Clause

#include "level0.h"

#include "../gpu/api/intel/level0/level0-api.h"
#include "common.h"
#include "level0-private.h"

const struct hpcrun_foil_hookdispatch_level0* hpcrun_foil_fetch_hooks_level0() {
  static const struct hpcrun_foil_hookdispatch_level0 hooks = {
      .zeInit = hpcrun_zeInit,
      .zeCommandListAppendLaunchKernel = hpcrun_zeCommandListAppendLaunchKernel,
      .zeCommandListAppendMemoryCopy = hpcrun_zeCommandListAppendMemoryCopy,
      .zeCommandListCreate = hpcrun_zeCommandListCreate,
      .zeCommandListCreateImmediate = hpcrun_zeCommandListCreateImmediate,
      .zeCommandListDestroy = hpcrun_zeCommandListDestroy,
      .zeCommandListReset = hpcrun_zeCommandListReset,
      .zeCommandQueueExecuteCommandLists = hpcrun_zeCommandQueueExecuteCommandLists,
      .zeEventPoolCreate = hpcrun_zeEventPoolCreate,
      .zeEventDestroy = hpcrun_zeEventDestroy,
      .zeEventHostReset = hpcrun_zeEventHostReset,
      .zeModuleCreate = hpcrun_zeModuleCreate,
      .zeModuleDestroy = hpcrun_zeModuleDestroy,
      .zeKernelCreate = hpcrun_zeKernelCreate,
      .zeKernelDestroy = hpcrun_zeKernelDestroy,
      .zeFenceDestroy = hpcrun_zeFenceDestroy,
      .zeFenceReset = hpcrun_zeFenceReset,
      .zeCommandQueueSynchronize = hpcrun_zeCommandQueueSynchronize,
  };
  return &hooks;
}

ze_result_t f_zeInit(ze_init_flag_t flag,
                     const struct hpcrun_foil_appdispatch_level0* dispatch) {
  return dispatch->zeInit(flag);
}

ze_result_t f_zeDriverGet(uint32_t* pCount, ze_driver_handle_t* phDrivers,
                          const struct hpcrun_foil_appdispatch_level0* dispatch) {
  return dispatch->zeDriverGet(pCount, phDrivers);
}

ze_result_t f_zeDeviceGet(ze_driver_handle_t hDriver, uint32_t* pCount,
                          ze_device_handle_t* phDevices,
                          const struct hpcrun_foil_appdispatch_level0* dispatch) {
  return dispatch->zeDeviceGet(hDriver, pCount, phDevices);
}

ze_result_t
f_zeDeviceGetProperties(ze_device_handle_t hDevice,
                        ze_device_properties_t* pDeviceProperties,
                        const struct hpcrun_foil_appdispatch_level0* dispatch) {
  return dispatch->zeDeviceGetProperties(hDevice, pDeviceProperties);
}

ze_result_t f_zeEventCreate(ze_event_pool_handle_t hEventPool,
                            const ze_event_desc_t* desc, ze_event_handle_t* phEvent,
                            const struct hpcrun_foil_appdispatch_level0* dispatch) {
  return dispatch->zeEventCreate(hEventPool, desc, phEvent);
}

ze_result_t f_zeEventDestroy(ze_event_handle_t hEvent,
                             const struct hpcrun_foil_appdispatch_level0* dispatch) {
  return dispatch->zeEventDestroy(hEvent);
}

ze_result_t f_zeEventPoolCreate(ze_context_handle_t hContext,
                                const ze_event_pool_desc_t* desc, uint32_t numDevices,
                                ze_device_handle_t* phDevices,
                                ze_event_pool_handle_t* phEventPool,
                                const struct hpcrun_foil_appdispatch_level0* dispatch) {
  return dispatch->zeEventPoolCreate(hContext, desc, numDevices, phDevices,
                                     phEventPool);
}

ze_result_t
f_zeEventPoolDestroy(ze_event_pool_handle_t hEventPool,
                     const struct hpcrun_foil_appdispatch_level0* dispatch) {
  return dispatch->zeEventPoolDestroy(hEventPool);
}

ze_result_t
f_zeEventQueryStatus(ze_event_handle_t hEvent,
                     const struct hpcrun_foil_appdispatch_level0* dispatch) {
  return dispatch->zeEventQueryStatus(hEvent);
}

ze_result_t
f_zeEventQueryKernelTimestamp(ze_event_handle_t hEvent,
                              ze_kernel_timestamp_result_t* dstptr,
                              const struct hpcrun_foil_appdispatch_level0* dispatch) {
  return dispatch->zeEventQueryKernelTimestamp(hEvent, dstptr);
}

ze_result_t
f_zeMemGetAllocProperties(ze_context_handle_t hContext, const void* ptr,
                          ze_memory_allocation_properties_t* pMemAllocProperties,
                          ze_device_handle_t* phDevice,
                          const struct hpcrun_foil_appdispatch_level0* dispatch) {
  return dispatch->zeMemGetAllocProperties(hContext, ptr, pMemAllocProperties,
                                           phDevice);
}

ze_result_t f_zeCommandListAppendLaunchKernel(
    ze_command_list_handle_t hCommandList, ze_kernel_handle_t hKernel,
    const ze_group_count_t* pLaunchFuncArgs, ze_event_handle_t hSignalEvent,
    uint32_t numWaitEvents, ze_event_handle_t* phWaitEvents,
    const struct hpcrun_foil_appdispatch_level0* dispatch) {
  return dispatch->zeCommandListAppendLaunchKernel(hCommandList, hKernel,
                                                   pLaunchFuncArgs, hSignalEvent,
                                                   numWaitEvents, phWaitEvents);
}

ze_result_t
f_zeCommandListAppendMemoryCopy(ze_command_list_handle_t hCommandList, void* dstptr,
                                const void* srcptr, size_t size,
                                ze_event_handle_t hSignalEvent, uint32_t numWaitEvents,
                                ze_event_handle_t* phWaitEvents,
                                const struct hpcrun_foil_appdispatch_level0* dispatch) {
  return dispatch->zeCommandListAppendMemoryCopy(
      hCommandList, dstptr, srcptr, size, hSignalEvent, numWaitEvents, phWaitEvents);
}

ze_result_t
f_zeCommandListCreate(ze_context_handle_t hContext, ze_device_handle_t hDevice,
                      const ze_command_list_desc_t* desc,
                      ze_command_list_handle_t* phCommandList,
                      const struct hpcrun_foil_appdispatch_level0* dispatch) {
  return dispatch->zeCommandListCreate(hContext, hDevice, desc, phCommandList);
}

ze_result_t
f_zeCommandListCreateImmediate(ze_context_handle_t hContext, ze_device_handle_t hDevice,
                               const ze_command_queue_desc_t* desc,
                               ze_command_list_handle_t* phCommandList,
                               const struct hpcrun_foil_appdispatch_level0* dispatch) {
  return dispatch->zeCommandListCreateImmediate(hContext, hDevice, desc, phCommandList);
}

ze_result_t
f_zeCommandListDestroy(ze_command_list_handle_t hCommandList,
                       const struct hpcrun_foil_appdispatch_level0* dispatch) {
  return dispatch->zeCommandListDestroy(hCommandList);
}

ze_result_t
f_zeCommandListReset(ze_command_list_handle_t hCommandList,
                     const struct hpcrun_foil_appdispatch_level0* dispatch) {
  return dispatch->zeCommandListReset(hCommandList);
}

ze_result_t f_zeCommandQueueExecuteCommandLists(
    ze_command_queue_handle_t hCommandQueue, uint32_t numCommandLists,
    ze_command_list_handle_t* phCommandLists, ze_fence_handle_t hFence,
    const struct hpcrun_foil_appdispatch_level0* dispatch) {
  return dispatch->zeCommandQueueExecuteCommandLists(hCommandQueue, numCommandLists,
                                                     phCommandLists, hFence);
}

ze_result_t f_zeEventHostReset(ze_event_handle_t hEvent,
                               const struct hpcrun_foil_appdispatch_level0* dispatch) {
  return dispatch->zeEventHostReset(hEvent);
}

ze_result_t f_zeModuleCreate(ze_context_handle_t hContext, ze_device_handle_t hDevice,
                             const ze_module_desc_t* desc, ze_module_handle_t* phModule,
                             ze_module_build_log_handle_t* phBuildLog,
                             const struct hpcrun_foil_appdispatch_level0* dispatch) {
  return dispatch->zeModuleCreate(hContext, hDevice, desc, phModule, phBuildLog);
}

ze_result_t f_zeModuleDestroy(ze_module_handle_t hModule,
                              const struct hpcrun_foil_appdispatch_level0* dispatch) {
  return dispatch->zeModuleDestroy(hModule);
}

ze_result_t f_zeKernelCreate(ze_module_handle_t hModule, const ze_kernel_desc_t* desc,
                             ze_kernel_handle_t* phKernel,
                             const struct hpcrun_foil_appdispatch_level0* dispatch) {
  return dispatch->zeKernelCreate(hModule, desc, phKernel);
}

ze_result_t f_zeKernelDestroy(ze_kernel_handle_t hKernel,
                              const struct hpcrun_foil_appdispatch_level0* dispatch) {
  return dispatch->zeKernelDestroy(hKernel);
}

ze_result_t f_zeFenceDestroy(ze_fence_handle_t hFence,
                             const struct hpcrun_foil_appdispatch_level0* dispatch) {
  return dispatch->zeFenceDestroy(hFence);
}

ze_result_t f_zeFenceReset(ze_fence_handle_t hFence,
                           const struct hpcrun_foil_appdispatch_level0* dispatch) {
  return dispatch->zeFenceReset(hFence);
}

ze_result_t
f_zeCommandQueueSynchronize(ze_command_queue_handle_t hCommandQueue, uint64_t timeout,
                            const struct hpcrun_foil_appdispatch_level0* dispatch) {
  return dispatch->zeCommandQueueSynchronize(hCommandQueue, timeout);
}

ze_result_t f_zeKernelGetName(ze_kernel_handle_t hKernel, size_t* pSize, char* pName,
                              const struct hpcrun_foil_appdispatch_level0* dispatch) {
  return dispatch->zeKernelGetName(hKernel, pSize, pName);
}

ze_result_t f_zetModuleGetDebugInfo(
    zet_module_handle_t hModule, zet_module_debug_info_format_t format, size_t* pSize,
    uint8_t* pDebugInfo, const struct hpcrun_foil_appdispatch_level0* dispatch) {
  return dispatch->zetModuleGetDebugInfo(hModule, format, pSize, pDebugInfo);
}

ze_result_t f_zetMetricGroupGetProperties(
    zet_metric_group_handle_t hMetricGroup, zet_metric_group_properties_t* pProperties,
    const struct hpcrun_foil_appdispatch_level0* dispatch) {
  return dispatch->zetMetricGroupGetProperties(hMetricGroup, pProperties);
}

ze_result_t f_zeContextCreate(
    ze_driver_handle_t hDriver, const ze_context_desc_t* desc,
    ze_context_handle_t* phContext, const struct hpcrun_foil_appdispatch_level0* dispatch) {
  return dispatch->zeContextCreate(hDriver, desc, phContext);
}

ze_result_t f_zeDeviceGetSubDevices(
    ze_device_handle_t hDevice, uint32_t* pCount, ze_device_handle_t* phSubdevices,
    const struct hpcrun_foil_appdispatch_level0* dispatch) {
  return dispatch->zeDeviceGetSubDevices(hDevice, pCount, phSubdevices);
}

ze_result_t f_zeDeviceGetRootDevice(
    ze_device_handle_t hDevice, ze_device_handle_t* phRootDevice,
    const struct hpcrun_foil_appdispatch_level0* dispatch) {
  return dispatch->zeDeviceGetRootDevice(hDevice, phRootDevice);
}

ze_result_t f_zeDriverGetApiVersion(
    ze_driver_handle_t hDriver, ze_api_version_t* version,
    const struct hpcrun_foil_appdispatch_level0* dispatch) {
  return dispatch->zeDriverGetApiVersion(hDriver, version);
}

ze_result_t f_zeEventHostSynchronize(
    ze_event_handle_t hEvent, uint64_t timeout,
    const struct hpcrun_foil_appdispatch_level0* dispatch) {
  return dispatch->zeEventHostSynchronize(hEvent, timeout);
}

ze_result_t f_zeEventHostSignal(ze_event_handle_t hEvent,
    const struct hpcrun_foil_appdispatch_level0* dispatch) {
  return dispatch->zeEventHostSignal(hEvent);
}

ze_result_t f_zeModuleGetKernelNames(ze_module_handle_t hModule,
    uint32_t* pCount, const char** pNames,
    const struct hpcrun_foil_appdispatch_level0* dispatch) {
  return dispatch->zeModuleGetKernelNames(hModule, pCount, pNames);
}

ze_result_t f_zeModuleGetFunctionPointer(ze_module_handle_t hModule,
    const char* pFunctionName, void** pfnFunction,
    const struct hpcrun_foil_appdispatch_level0* dispatch) {
  return dispatch->zeModuleGetFunctionPointer(hModule, pFunctionName, pfnFunction);
}

ze_result_t f_zeKernelGetProperties(ze_kernel_handle_t hKernel,
    ze_kernel_properties_t* pKernelProperties,
    const struct hpcrun_foil_appdispatch_level0* dispatch) {
  return dispatch->zeKernelGetProperties(hKernel, pKernelProperties);
}

ze_result_t f_zeCommandListGetDeviceHandle(ze_command_list_handle_t hCommandList,
    ze_device_handle_t* phDevice, const struct hpcrun_foil_appdispatch_level0* dispatch) {
  return dispatch->zeCommandListGetDeviceHandle(hCommandList, phDevice);
}

ze_result_t f_zetMetricGet(zet_metric_group_handle_t hMetricGroup,
    uint32_t* pCount, zet_metric_handle_t* phMetrics,
    const struct hpcrun_foil_appdispatch_level0* dispatch) {
  return dispatch->zetMetricGet(hMetricGroup, pCount, phMetrics);
}

ze_result_t f_zetMetricGetProperties(zet_metric_handle_t hMetric,
    zet_metric_properties_t* pProperties, const struct hpcrun_foil_appdispatch_level0* dispatch) {
  return dispatch->zetMetricGetProperties(hMetric, pProperties);
}

ze_result_t f_zetContextActivateMetricGroups(zet_context_handle_t hContext,
    ze_device_handle_t hDevice, uint32_t count, zet_metric_group_handle_t* phMetricGroups,
    const struct hpcrun_foil_appdispatch_level0* dispatch) {
  return dispatch->zetContextActivateMetricGroups(hContext, hDevice, count, phMetricGroups);
}

ze_result_t f_zetMetricStreamerOpen(zet_context_handle_t hContext,
    ze_device_handle_t hDevice, zet_metric_group_handle_t hMetricGroup,
    zet_metric_streamer_desc_t* desc, ze_event_handle_t hNotificationEvent,
    zet_metric_streamer_handle_t* phMetricStreamer,
    const struct hpcrun_foil_appdispatch_level0* dispatch) {
  return dispatch->zetMetricStreamerOpen(hContext, hDevice, hMetricGroup, desc, hNotificationEvent, phMetricStreamer);
}

ze_result_t f_zetMetricStreamerClose(zet_metric_streamer_handle_t hMetricStreamer,
    const struct hpcrun_foil_appdispatch_level0* dispatch) {
  return dispatch->zetMetricStreamerClose(hMetricStreamer);
}

ze_result_t f_zetMetricGroupGet(zet_device_handle_t hDevice,
    uint32_t* pCount, zet_metric_group_handle_t* phMetricGroups,
    const struct hpcrun_foil_appdispatch_level0* dispatch) {
  return dispatch->zetMetricGroupGet(hDevice, pCount, phMetricGroups);
}

ze_result_t f_zetMetricStreamerReadData(zet_metric_streamer_handle_t hMetricStreamer,
    uint32_t maxReportCount, size_t* pRawDataSize, uint8_t* pRawData,
    const struct hpcrun_foil_appdispatch_level0* dispatch) {
  return dispatch->zetMetricStreamerReadData(hMetricStreamer, maxReportCount, pRawDataSize, pRawData);
}

ze_result_t f_zetMetricGroupCalculateMultipleMetricValuesExp(
    zet_metric_group_handle_t hMetricGroup, zet_metric_group_calculation_type_t type,
    size_t rawDataSize, const uint8_t *pRawData, uint32_t *pSetCount,
    uint32_t *pTotalMetricValueCount, uint32_t *pMetricCounts, zet_typed_value_t *pMetricValues,
    const struct hpcrun_foil_appdispatch_level0* dispatch) {
  return dispatch->zetMetricGroupCalculateMultipleMetricValuesExp(
    hMetricGroup, type,
    rawDataSize, pRawData, pSetCount, pTotalMetricValueCount, 
    pMetricCounts, pMetricValues);
}

ze_result_t f_zelTracerSetPrologues(zel_tracer_handle_t tracer,
    zel_core_callbacks_t* prologues,
    const struct hpcrun_foil_appdispatch_level0* dispatch) {
  return dispatch->zelTracerSetPrologues(tracer, prologues);
}

ze_result_t f_zelTracerSetEpilogues(zel_tracer_handle_t tracer,
    zel_core_callbacks_t* epilogues,
    const struct hpcrun_foil_appdispatch_level0* dispatch) {
  return dispatch->zelTracerSetEpilogues(tracer, epilogues);
}

ze_result_t f_zelTracerSetEnabled(zel_tracer_handle_t tracer,
    ze_bool_t enable,
    const struct hpcrun_foil_appdispatch_level0* dispatch) {
  return dispatch->zelTracerSetEnabled(tracer, enable);
}

ze_result_t f_zelTracerCreate(const zel_tracer_desc_t* desc,
    zel_tracer_handle_t* tracer,
    const struct hpcrun_foil_appdispatch_level0* dispatch) {
  return dispatch->zelTracerCreate(desc, tracer);
}

ze_result_t f_zelTracerDestroy(zel_tracer_handle_t tracer,
    const struct hpcrun_foil_appdispatch_level0* dispatch) {
  return dispatch->zelTracerDestroy(tracer);
}
