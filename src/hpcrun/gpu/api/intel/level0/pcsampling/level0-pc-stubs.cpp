// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: Apache-2.0
//
// SPDX-License-Identifier: MIT
// Copyright (C) 2024 Intel Corporation
// This file was inspired by and uses some code fragments from Intel's
// MIT-licensed pti-gpu (https://github.com/intel/pti-gpu)

// -*-Mode: C++;-*-

//*****************************************************************************
// level zero includes
//*****************************************************************************

#include <level_zero/ze_api.h>
#include <level_zero/zet_api.h>
#include <level_zero/layers/zel_tracing_api.h>


//*****************************************************************************
// local includes
//*****************************************************************************

#include "../level0-pc-hpcrun-api.h"
#include "level0-pc-api-receiver.hpp"

namespace {

inline level0_pc_hpcrun_api_t* api()
{
  return pcsampling::getHpcrunApi();
}

template <typename Func, typename... Args>
inline ze_result_t invoke(Func fn, Args... args)
{
  if (fn) {
    return fn(args...);
  }
  return ZE_RESULT_ERROR_UNINITIALIZED;
}

} // namespace

extern "C" ze_result_t
f_zeDriverGet(uint32_t* pCount, ze_driver_handle_t* phDrivers,
              const struct hpcrun_foil_appdispatch_level0* dispatch)
{
  auto p = api();
  return p ? invoke(p->f_zeDriverGet, pCount, phDrivers, dispatch)
           : ZE_RESULT_ERROR_UNINITIALIZED;
}

extern "C" ze_result_t
f_zeDriverGetApiVersion(ze_driver_handle_t hDriver, ze_api_version_t* version,
                        const struct hpcrun_foil_appdispatch_level0* dispatch)
{
  auto p = api();
  return p ? invoke(p->f_zeDriverGetApiVersion, hDriver, version, dispatch)
           : ZE_RESULT_ERROR_UNINITIALIZED;
}

extern "C" ze_result_t
f_zeContextCreate(ze_driver_handle_t hDriver, const ze_context_desc_t* desc,
                  ze_context_handle_t* phContext,
                  const struct hpcrun_foil_appdispatch_level0* dispatch)
{
  auto p = api();
  return p ? invoke(p->f_zeContextCreate, hDriver, desc, phContext, dispatch)
           : ZE_RESULT_ERROR_UNINITIALIZED;
}

extern "C" ze_result_t
f_zeContextDestroy(ze_context_handle_t hContext,
                   const struct hpcrun_foil_appdispatch_level0* dispatch)
{
  auto p = api();
  return p ? invoke(p->f_zeContextDestroy, hContext, dispatch)
           : ZE_RESULT_ERROR_UNINITIALIZED;
}

extern "C" ze_result_t
f_zeContextMakeMemoryResident(ze_context_handle_t hContext, ze_device_handle_t hDevice,
                              void* ptr, size_t size,
                              const struct hpcrun_foil_appdispatch_level0* dispatch)
{
  auto p = api();
  return p ? invoke(p->f_zeContextMakeMemoryResident, hContext, hDevice, ptr, size, dispatch)
           : ZE_RESULT_ERROR_UNINITIALIZED;
}

extern "C" ze_result_t
f_zeContextEvictMemory(ze_context_handle_t hContext, ze_device_handle_t hDevice,
                       void* ptr, size_t size,
                       const struct hpcrun_foil_appdispatch_level0* dispatch)
{
  auto p = api();
  return p ? invoke(p->f_zeContextEvictMemory, hContext, hDevice, ptr, size, dispatch)
           : ZE_RESULT_ERROR_UNINITIALIZED;
}

extern "C" ze_result_t
f_zeDeviceGet(ze_driver_handle_t hDriver, uint32_t* pCount,
              ze_device_handle_t* phDevices,
              const struct hpcrun_foil_appdispatch_level0* dispatch)
{
  auto p = api();
  return p ? invoke(p->f_zeDeviceGet, hDriver, pCount, phDevices, dispatch)
           : ZE_RESULT_ERROR_UNINITIALIZED;
}

extern "C" ze_result_t
f_zeDeviceGetSubDevices(ze_device_handle_t hDevice, uint32_t* pCount,
                        ze_device_handle_t* phSubdevices,
                        const struct hpcrun_foil_appdispatch_level0* dispatch)
{
  auto p = api();
  return p ? invoke(p->f_zeDeviceGetSubDevices, hDevice, pCount, phSubdevices, dispatch)
           : ZE_RESULT_ERROR_UNINITIALIZED;
}

extern "C" ze_result_t
f_zeDeviceGetProperties(ze_device_handle_t hDevice,
                        ze_device_properties_t* pDeviceProperties,
                        const struct hpcrun_foil_appdispatch_level0* dispatch)
{
  auto p = api();
  return p ? invoke(p->f_zeDeviceGetProperties, hDevice, pDeviceProperties, dispatch)
           : ZE_RESULT_ERROR_UNINITIALIZED;
}

extern "C" ze_result_t
f_zeDeviceGetRootDevice(ze_device_handle_t hDevice,
                        ze_device_handle_t* phRootDevice,
                        const struct hpcrun_foil_appdispatch_level0* dispatch)
{
  auto p = api();
  return p ? invoke(p->f_zeDeviceGetRootDevice, hDevice, phRootDevice, dispatch)
           : ZE_RESULT_ERROR_UNINITIALIZED;
}

extern "C" ze_result_t
f_zeDriverGetExtensionFunctionAddress(ze_driver_handle_t hDriver, const char* name,
                                      void** ppFunctionAddress,
                                      const struct hpcrun_foil_appdispatch_level0* dispatch)
{
  auto p = api();
  return p ? invoke(p->f_zeDriverGetExtensionFunctionAddress, hDriver, name, ppFunctionAddress, dispatch)
           : ZE_RESULT_ERROR_UNINITIALIZED;
}

extern "C" ze_result_t
f_zeEventCreate(ze_event_pool_handle_t hEventPool, const ze_event_desc_t* desc,
                ze_event_handle_t* phEvent,
                const struct hpcrun_foil_appdispatch_level0* dispatch)
{
  auto p = api();
  return p ? invoke(p->f_zeEventCreate, hEventPool, desc, phEvent, dispatch)
           : ZE_RESULT_ERROR_UNINITIALIZED;
}

extern "C" ze_result_t
f_zeEventDestroy(ze_event_handle_t hEvent,
                 const struct hpcrun_foil_appdispatch_level0* dispatch)
{
  auto p = api();
  return p ? invoke(p->f_zeEventDestroy, hEvent, dispatch)
           : ZE_RESULT_ERROR_UNINITIALIZED;
}

extern "C" ze_result_t
f_zeEventPoolCreate(ze_context_handle_t hContext, const ze_event_pool_desc_t* desc,
                    uint32_t numDevices, ze_device_handle_t* phDevices,
                    ze_event_pool_handle_t* phEventPool,
                    const struct hpcrun_foil_appdispatch_level0* dispatch)
{
  auto p = api();
  return p ? invoke(p->f_zeEventPoolCreate, hContext, desc, numDevices, phDevices, phEventPool, dispatch)
           : ZE_RESULT_ERROR_UNINITIALIZED;
}

extern "C" ze_result_t
f_zeEventPoolDestroy(ze_event_pool_handle_t hEventPool,
                     const struct hpcrun_foil_appdispatch_level0* dispatch)
{
  auto p = api();
  return p ? invoke(p->f_zeEventPoolDestroy, hEventPool, dispatch)
           : ZE_RESULT_ERROR_UNINITIALIZED;
}

extern "C" ze_result_t
f_zeEventHostSynchronize(ze_event_handle_t hEvent, uint64_t timeout,
                         const struct hpcrun_foil_appdispatch_level0* dispatch)
{
  auto p = api();
  return p ? invoke(p->f_zeEventHostSynchronize, hEvent, timeout, dispatch)
           : ZE_RESULT_ERROR_UNINITIALIZED;
}

extern "C" ze_result_t
f_zeEventQueryStatus(ze_event_handle_t hEvent,
                     const struct hpcrun_foil_appdispatch_level0* dispatch)
{
  auto p = api();
  return p ? invoke(p->f_zeEventQueryStatus, hEvent, dispatch)
           : ZE_RESULT_ERROR_UNINITIALIZED;
}

extern "C" ze_result_t
f_zeEventQueryKernelTimestamp(ze_event_handle_t hEvent,
                              ze_kernel_timestamp_result_t* dstptr,
                              const struct hpcrun_foil_appdispatch_level0* dispatch)
{
  auto p = api();
  return p ? invoke(p->f_zeEventQueryKernelTimestamp, hEvent, dstptr, dispatch)
           : ZE_RESULT_ERROR_UNINITIALIZED;
}

extern "C" ze_result_t
f_zeKernelCreate(ze_module_handle_t hModule, const ze_kernel_desc_t* desc,
                 ze_kernel_handle_t* phKernel,
                 const struct hpcrun_foil_appdispatch_level0* dispatch)
{
  auto p = api();
  return p ? invoke(p->f_zeKernelCreate, hModule, desc, phKernel, dispatch)
           : ZE_RESULT_ERROR_UNINITIALIZED;
}

extern "C" ze_result_t
f_zeKernelDestroy(ze_kernel_handle_t hKernel,
                  const struct hpcrun_foil_appdispatch_level0* dispatch)
{
  auto p = api();
  return p ? invoke(p->f_zeKernelDestroy, hKernel, dispatch)
           : ZE_RESULT_ERROR_UNINITIALIZED;
}

extern "C" ze_result_t
f_zeKernelGetName(ze_kernel_handle_t hKernel, size_t* pSize, char* pName,
                  const struct hpcrun_foil_appdispatch_level0* dispatch)
{
  auto p = api();
  return p ? invoke(p->f_zeKernelGetName, hKernel, pSize, pName, dispatch)
           : ZE_RESULT_ERROR_UNINITIALIZED;
}

extern "C" ze_result_t
f_zeKernelGetProperties(ze_kernel_handle_t hKernel,
                        ze_kernel_properties_t* pKernelProperties,
                        const struct hpcrun_foil_appdispatch_level0* dispatch)
{
  auto p = api();
  return p ? invoke(p->f_zeKernelGetProperties, hKernel, pKernelProperties, dispatch)
           : ZE_RESULT_ERROR_UNINITIALIZED;
}

extern "C" ze_result_t
f_zeModuleCreate(ze_context_handle_t hContext, ze_device_handle_t hDevice,
                 const ze_module_desc_t* desc, ze_module_handle_t* phModule,
                 ze_module_build_log_handle_t* phBuildLog,
                 const struct hpcrun_foil_appdispatch_level0* dispatch)
{
  auto p = api();
  return p ? invoke(p->f_zeModuleCreate, hContext, hDevice, desc, phModule, phBuildLog, dispatch)
           : ZE_RESULT_ERROR_UNINITIALIZED;
}

extern "C" ze_result_t
f_zeModuleDestroy(ze_module_handle_t hModule,
                  const struct hpcrun_foil_appdispatch_level0* dispatch)
{
  auto p = api();
  return p ? invoke(p->f_zeModuleDestroy, hModule, dispatch)
           : ZE_RESULT_ERROR_UNINITIALIZED;
}

extern "C" ze_result_t
f_zeModuleGetFunctionPointer(ze_module_handle_t hModule, const char* pFunctionName,
                             void** pfnFunction,
                             const struct hpcrun_foil_appdispatch_level0* dispatch)
{
  auto p = api();
  return p ? invoke(p->f_zeModuleGetFunctionPointer, hModule, pFunctionName, pfnFunction, dispatch)
           : ZE_RESULT_ERROR_UNINITIALIZED;
}

extern "C" ze_result_t
f_zeModuleGetKernelNames(ze_module_handle_t hModule, uint32_t* pCount,
                          const char** pNames,
                          const struct hpcrun_foil_appdispatch_level0* dispatch)
{
  auto p = api();
  return p ? invoke(p->f_zeModuleGetKernelNames, hModule, pCount, pNames, dispatch)
           : ZE_RESULT_ERROR_UNINITIALIZED;
}

extern "C" ze_result_t
f_zeCommandListCreate(ze_context_handle_t hContext, ze_device_handle_t hDevice,
                      const ze_command_list_desc_t* desc, ze_command_list_handle_t* phCommandList,
                      const struct hpcrun_foil_appdispatch_level0* dispatch)
{
  auto p = api();
  return p ? invoke(p->f_zeCommandListCreate, hContext, hDevice, desc, phCommandList, dispatch)
           : ZE_RESULT_ERROR_UNINITIALIZED;
}

extern "C" ze_result_t
f_zeCommandListDestroy(ze_command_list_handle_t hCommandList,
                       const struct hpcrun_foil_appdispatch_level0* dispatch)
{
  auto p = api();
  return p ? invoke(p->f_zeCommandListDestroy, hCommandList, dispatch)
           : ZE_RESULT_ERROR_UNINITIALIZED;
}

extern "C" ze_result_t
f_zeCommandListReset(ze_command_list_handle_t hCommandList,
                     const struct hpcrun_foil_appdispatch_level0* dispatch)
{
  auto p = api();
  return p ? invoke(p->f_zeCommandListReset, hCommandList, dispatch)
           : ZE_RESULT_ERROR_UNINITIALIZED;
}

extern "C" ze_result_t
f_zeCommandListClose(ze_command_list_handle_t hCommandList,
                     const struct hpcrun_foil_appdispatch_level0* dispatch)
{
  auto p = api();
  return p ? invoke(p->f_zeCommandListClose, hCommandList, dispatch)
           : ZE_RESULT_ERROR_UNINITIALIZED;
}

extern "C" ze_result_t
f_zeCommandListGetCommandQueueGroupOrdinal(ze_command_list_handle_t hCommandList,
                                           uint32_t* pOrdinal,
                                           const struct hpcrun_foil_appdispatch_level0* dispatch)
{
  auto p = api();
  return p ? invoke(p->f_zeCommandListGetCommandQueueGroupOrdinal, hCommandList, pOrdinal, dispatch)
           : ZE_RESULT_ERROR_UNINITIALIZED;
}

extern "C" ze_result_t
f_zeCommandListGetDeviceHandle(ze_command_list_handle_t hCommandList,
                               ze_device_handle_t* phDevice,
                               const struct hpcrun_foil_appdispatch_level0* dispatch)
{
  auto p = api();
  return p ? invoke(p->f_zeCommandListGetDeviceHandle, hCommandList, phDevice, dispatch)
           : ZE_RESULT_ERROR_UNINITIALIZED;
}

extern "C" ze_result_t
f_zeCommandListHostSynchronize(ze_command_list_handle_t hCommandList, uint64_t timeout,
                               const struct hpcrun_foil_appdispatch_level0* dispatch)
{
  auto p = api();
  return p ? invoke(p->f_zeCommandListHostSynchronize, hCommandList, timeout, dispatch)
           : ZE_RESULT_ERROR_UNINITIALIZED;
}

extern "C" ze_result_t
f_zeCommandListAppendBarrier(ze_command_list_handle_t hCommandList, ze_event_handle_t hSignalEvent,
                             uint32_t numWaitEvents, ze_event_handle_t* phWaitEvents,
                             const struct hpcrun_foil_appdispatch_level0* dispatch)
{
  auto p = api();
  return p ? invoke(p->f_zeCommandListAppendBarrier, hCommandList, hSignalEvent,
                    numWaitEvents, phWaitEvents, dispatch)
           : ZE_RESULT_ERROR_UNINITIALIZED;
}

extern "C" ze_result_t
f_zetContextActivateMetricGroups(zet_context_handle_t hContext, ze_device_handle_t hDevice,
                                 uint32_t count, zet_metric_group_handle_t* phMetricGroups,
                                 const struct hpcrun_foil_appdispatch_level0* dispatch)
{
  auto p = api();
  return p ? invoke(p->f_zetContextActivateMetricGroups, hContext, hDevice,
                    count, phMetricGroups, dispatch)
           : ZE_RESULT_ERROR_UNINITIALIZED;
}

extern "C" ze_result_t
f_zetMetricGet(zet_metric_group_handle_t hMetricGroup, uint32_t* pCount,
               zet_metric_handle_t* phMetrics,
               const struct hpcrun_foil_appdispatch_level0* dispatch)
{
  auto p = api();
  return p ? invoke(p->f_zetMetricGet, hMetricGroup, pCount, phMetrics, dispatch)
           : ZE_RESULT_ERROR_UNINITIALIZED;
}

extern "C" ze_result_t
f_zetMetricGetProperties(zet_metric_handle_t hMetric,
                         zet_metric_properties_t* pProperties,
                         const struct hpcrun_foil_appdispatch_level0* dispatch)
{
  auto p = api();
  return p ? invoke(p->f_zetMetricGetProperties, hMetric, pProperties, dispatch)
           : ZE_RESULT_ERROR_UNINITIALIZED;
}

extern "C" ze_result_t
f_zetMetricGroupCalculateMultipleMetricValuesExp(
    zet_metric_group_handle_t hMetricGroup,
    zet_metric_group_calculation_type_t type,
    size_t rawDataSize,
    const uint8_t* pRawData,
    uint32_t* pSetCount,
    uint32_t* pTotalMetricValueCount,
    uint32_t* pMetricCounts,
    zet_typed_value_t* pMetricValues,
    const struct hpcrun_foil_appdispatch_level0* dispatch)
{
  auto p = api();
  return p ? invoke(p->f_zetMetricGroupCalculateMultipleMetricValuesExp, hMetricGroup, type,
                    rawDataSize, pRawData, pSetCount, pTotalMetricValueCount,
                    pMetricCounts, pMetricValues, dispatch)
           : ZE_RESULT_ERROR_UNINITIALIZED;
}

extern "C" ze_result_t
f_zetMetricGroupGet(ze_device_handle_t hDevice, uint32_t* pCount,
                    zet_metric_group_handle_t* phMetricGroups,
                    const struct hpcrun_foil_appdispatch_level0* dispatch)
{
  auto p = api();
  return p ? invoke(p->f_zetMetricGroupGet, hDevice, pCount, phMetricGroups, dispatch)
           : ZE_RESULT_ERROR_UNINITIALIZED;
}

extern "C" ze_result_t
f_zetMetricGroupGetProperties(zet_metric_group_handle_t hMetricGroup,
                              zet_metric_group_properties_t* pProperties,
                              const struct hpcrun_foil_appdispatch_level0* dispatch)
{
  auto p = api();
  return p ? invoke(p->f_zetMetricGroupGetProperties, hMetricGroup, pProperties, dispatch)
           : ZE_RESULT_ERROR_UNINITIALIZED;
}

extern "C" ze_result_t
f_zetMetricStreamerOpen(ze_context_handle_t hContext, ze_device_handle_t hDevice,
                        zet_metric_group_handle_t hMetricGroup,
                        zet_metric_streamer_desc_t* desc, ze_event_handle_t hNotificationEvent,
                        zet_metric_streamer_handle_t* phMetricStreamer,
                        const struct hpcrun_foil_appdispatch_level0* dispatch)
{
  auto p = api();
  return p ? invoke(p->f_zetMetricStreamerOpen, hContext, hDevice, hMetricGroup, desc,
                    hNotificationEvent, phMetricStreamer, dispatch)
           : ZE_RESULT_ERROR_UNINITIALIZED;
}

extern "C" ze_result_t
f_zetMetricStreamerClose(zet_metric_streamer_handle_t hMetricStreamer,
                         const struct hpcrun_foil_appdispatch_level0* dispatch)
{
  auto p = api();
  return p ? invoke(p->f_zetMetricStreamerClose, hMetricStreamer, dispatch)
           : ZE_RESULT_ERROR_UNINITIALIZED;
}

extern "C" ze_result_t
f_zetMetricStreamerReadData(zet_metric_streamer_handle_t hMetricStreamer,
                            uint32_t maxReportCount, size_t* pRawDataSize, uint8_t* pRawData,
                            const struct hpcrun_foil_appdispatch_level0* dispatch)
{
  auto p = api();
  return p ? invoke(p->f_zetMetricStreamerReadData, hMetricStreamer, maxReportCount,
                    pRawDataSize, pRawData, dispatch)
           : ZE_RESULT_ERROR_UNINITIALIZED;
}

extern "C" ze_result_t
f_zetModuleGetDebugInfo(ze_module_handle_t hModule, zet_module_debug_info_format_t format,
                        size_t* pSize, uint8_t* pDebugInfo,
                        const struct hpcrun_foil_appdispatch_level0* dispatch)
{
  auto p = api();
  return p ? invoke(p->f_zetModuleGetDebugInfo, hModule, format, pSize, pDebugInfo, dispatch)
           : ZE_RESULT_ERROR_UNINITIALIZED;
}

extern "C" ze_result_t
f_zelTracerCreate(const zel_tracer_desc_t* desc, zel_tracer_handle_t* phTracer,
                  const struct hpcrun_foil_appdispatch_level0* dispatch)
{
  auto p = api();
  return p ? invoke(p->f_zelTracerCreate, desc, phTracer, dispatch)
           : ZE_RESULT_ERROR_UNINITIALIZED;
}

extern "C" ze_result_t
f_zelTracerDestroy(zel_tracer_handle_t hTracer,
                   const struct hpcrun_foil_appdispatch_level0* dispatch)
{
  auto p = api();
  return p ? invoke(p->f_zelTracerDestroy, hTracer, dispatch)
           : ZE_RESULT_ERROR_UNINITIALIZED;
}

extern "C" ze_result_t
f_zelTracerSetPrologues(zel_tracer_handle_t hTracer, zel_core_callbacks_t* pCoreCbs,
                        const struct hpcrun_foil_appdispatch_level0* dispatch)
{
  auto p = api();
  return p ? invoke(p->f_zelTracerSetPrologues, hTracer, pCoreCbs, dispatch)
           : ZE_RESULT_ERROR_UNINITIALIZED;
}

extern "C" ze_result_t
f_zelTracerSetEpilogues(zel_tracer_handle_t hTracer, zel_core_callbacks_t* pCoreCbs,
                        const struct hpcrun_foil_appdispatch_level0* dispatch)
{
  auto p = api();
  return p ? invoke(p->f_zelTracerSetEpilogues, hTracer, pCoreCbs, dispatch)
           : ZE_RESULT_ERROR_UNINITIALIZED;
}

extern "C" ze_result_t
f_zelTracerSetEnabled(zel_tracer_handle_t hTracer, ze_bool_t enable,
                      const struct hpcrun_foil_appdispatch_level0* dispatch)
{
  auto p = api();
  return p ? invoke(p->f_zelTracerSetEnabled, hTracer, enable, dispatch)
           : ZE_RESULT_ERROR_UNINITIALIZED;
}

// End of file
