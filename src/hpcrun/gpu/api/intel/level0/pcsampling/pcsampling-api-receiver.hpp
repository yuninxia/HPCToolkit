// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: BSD-3-Clause

// -*-Mode: C++;-*-

#ifndef _PCSAMPLING_API_RECEIVER_HPP_
#define _PCSAMPLING_API_RECEIVER_HPP_

//*****************************************************************************
// system includes
//*****************************************************************************

#include <cstddef>
#include <cstdint>
#include <cstdarg>


//*****************************************************************************
// local includes
//*****************************************************************************

#include "../pcsampling-hpcrun-api.h"
#include "../../../../../../common/lean/mcs-lock.h"


//*****************************************************************************
// namespace declaration
//*****************************************************************************

namespace pcsampling {

//*****************************************************************************
// interface functions
//*****************************************************************************

// Check if API has been initialized
bool isInitialized();
pcsampling_hpcrun_api_t* getHpcrunApi();

//-----------------------------------------------------------------------------
// Memory management wrappers
//-----------------------------------------------------------------------------

gpu_activity_t* allocActivity();
void freeActivity(gpu_activity_t* activity);
void initActivity(gpu_activity_t* activity);
gpu_activity_t** allocActivityBatch(size_t count);
void freeActivityBatch(gpu_activity_t** activities, size_t count);
void* allocMemory(size_t size);
void freeMemory(void* ptr);

//-----------------------------------------------------------------------------
// Thread management wrappers
//-----------------------------------------------------------------------------

int createProfilingThread(void* (*thread_func)(void*), void* arg, const char* name);
void joinProfilingThread(int thread_id);
void disableNewThreads();
void enableNewThreads();

//-----------------------------------------------------------------------------
// Error handling wrappers
//-----------------------------------------------------------------------------

void reportError(pcsampling_result_t error_code, const char* message,
                const char* file, int line);
void reportWarning(const char* fmt, ...);

// Macro for easier error reporting
#define PCSAMPLING_ERROR(code, msg) \
    pcsampling::reportError(code, msg, __FILE__, __LINE__)

//-----------------------------------------------------------------------------
// Safe entry/exit wrappers
//-----------------------------------------------------------------------------

int safeEnter();
void safeExit();

//-----------------------------------------------------------------------------
// GPU activity channel wrappers
//-----------------------------------------------------------------------------

gpu_activity_channel_t* lookupActivityChannel(uint32_t thread_id);
void sendActivity(gpu_activity_channel_t* channel, gpu_activity_t* activity);
void sendActivityDirect(uint64_t correlation_id, gpu_activity_t* activity);
uint32_t getThreadIdFromCorrelationId(uint64_t cid);

//-----------------------------------------------------------------------------
// Correlation channel wrappers
//-----------------------------------------------------------------------------

void sendCorrelation(int device_id, uint64_t cid, gpu_activity_channel_t* channel);
void receiveCorrelation(int device_id, correlation_handler_fn_t handler, void* arg);
void receiveCorrelationChannel(int device_id, correlation_handler_fn_t handler, void* arg);

//-----------------------------------------------------------------------------
// Synchronization wrappers
//-----------------------------------------------------------------------------

void* allocMutex();
void freeMutex(void* mutex);
void lockMutex(void* mutex);
void unlockMutex(void* mutex);

//-----------------------------------------------------------------------------
// Utility wrappers
//-----------------------------------------------------------------------------

uint64_t nanotime();
void warn(const char* fmt, ...);
void error(const char* fmt, ...);
void trace(int level, const char* fmt, ...);
const char* getEnvironmentVariable(const char* name);
int computeHashString(const void* data, size_t size, char* out, unsigned int out_len);
void mcsLock(mcs_lock_t* lock, mcs_node_t* node);
void mcsUnlock(mcs_lock_t* lock, mcs_node_t* node);

//-----------------------------------------------------------------------------
// Level Zero function wrappers
//-----------------------------------------------------------------------------

ze_result_t callZeDriverGet(uint32_t* pCount, ze_driver_handle_t* phDrivers,
                           const struct hpcrun_foil_appdispatch_level0* dispatch);
ze_result_t callZeDriverGetApiVersion(ze_driver_handle_t hDriver, ze_api_version_t* version,
                                     const struct hpcrun_foil_appdispatch_level0* dispatch);
ze_result_t callZeContextCreate(ze_driver_handle_t hDriver, const ze_context_desc_t* desc,
                               ze_context_handle_t* phContext,
                               const struct hpcrun_foil_appdispatch_level0* dispatch);
ze_result_t callZeDeviceGet(ze_driver_handle_t hDriver, uint32_t* pCount,
                           ze_device_handle_t* phDevices,
                           const struct hpcrun_foil_appdispatch_level0* dispatch);
ze_result_t callZeDeviceGetSubDevices(ze_device_handle_t hDevice, uint32_t* pCount,
                                     ze_device_handle_t* phSubdevices,
                                     const struct hpcrun_foil_appdispatch_level0* dispatch);
ze_result_t callZeDeviceGetProperties(ze_device_handle_t hDevice,
                                     ze_device_properties_t* pDeviceProperties,
                                     const struct hpcrun_foil_appdispatch_level0* dispatch);
ze_result_t callZeDeviceGetRootDevice(ze_device_handle_t hDevice,
                                     ze_device_handle_t* phRootDevice,
                                     const struct hpcrun_foil_appdispatch_level0* dispatch);
ze_result_t callZeDriverGetExtensionFunctionAddress(ze_driver_handle_t hDriver, const char* name,
                                                   void** ppFunctionAddress,
                                                   const struct hpcrun_foil_appdispatch_level0* dispatch);
ze_result_t callZeEventCreate(ze_event_pool_handle_t hEventPool, const ze_event_desc_t* desc,
                             ze_event_handle_t* phEvent,
                             const struct hpcrun_foil_appdispatch_level0* dispatch);
ze_result_t callZeEventPoolCreate(ze_context_handle_t hContext, const ze_event_pool_desc_t* desc,
                                 uint32_t numDevices, ze_device_handle_t* phDevices,
                                 ze_event_pool_handle_t* phEventPool,
                                 const struct hpcrun_foil_appdispatch_level0* dispatch);
ze_result_t callZeEventQueryStatus(ze_event_handle_t hEvent,
                                  const struct hpcrun_foil_appdispatch_level0* dispatch);
ze_result_t callZeEventQueryKernelTimestamp(ze_event_handle_t hEvent,
                                           ze_kernel_timestamp_result_t* dstptr,
                                           const struct hpcrun_foil_appdispatch_level0* dispatch);
ze_result_t callZeKernelGetName(ze_kernel_handle_t hKernel, size_t* pSize, char* pName,
                               const struct hpcrun_foil_appdispatch_level0* dispatch);
ze_result_t callZeKernelGetProperties(ze_kernel_handle_t hKernel,
                                     ze_kernel_properties_t* pKernelProperties,
                                     const struct hpcrun_foil_appdispatch_level0* dispatch);
ze_result_t callZeModuleGetFunctionPointer(ze_module_handle_t hModule, const char* pFunctionName,
                                          void** pfnFunction,
                                          const struct hpcrun_foil_appdispatch_level0* dispatch);
ze_result_t callZeModuleGetKernelNames(ze_module_handle_t hModule, uint32_t* pCount,
                                      const char** pNames,
                                      const struct hpcrun_foil_appdispatch_level0* dispatch);
ze_result_t callZeCommandListGetDeviceHandle(ze_command_list_handle_t hCommandList,
                                            ze_device_handle_t* phDevice,
                                            const struct hpcrun_foil_appdispatch_level0* dispatch);

//-----------------------------------------------------------------------------
// Level Zero Tools API wrappers
//-----------------------------------------------------------------------------

ze_result_t callZetContextActivateMetricGroups(zet_context_handle_t hContext, ze_device_handle_t hDevice,
                                              uint32_t count, zet_metric_group_handle_t* phMetricGroups,
                                              const struct hpcrun_foil_appdispatch_level0* dispatch);
ze_result_t callZetMetricGet(zet_metric_group_handle_t hMetricGroup, uint32_t* pCount,
                            zet_metric_handle_t* phMetrics,
                            const struct hpcrun_foil_appdispatch_level0* dispatch);
ze_result_t callZetMetricGetProperties(zet_metric_handle_t hMetric,
                                      zet_metric_properties_t* pProperties,
                                      const struct hpcrun_foil_appdispatch_level0* dispatch);
ze_result_t callZetMetricGroupCalculateMultipleMetricValuesExp(
                                      zet_metric_group_handle_t hMetricGroup,
                                      zet_metric_group_calculation_type_t type, size_t rawDataSize,
                                      const uint8_t* pRawData, uint32_t* pSetCount,
                                      uint32_t* pTotalMetricValueCount,
                                      uint32_t* pMetricCounts,
                                      zet_typed_value_t* pMetricValues,
                                      const struct hpcrun_foil_appdispatch_level0* dispatch);
ze_result_t callZetModuleGetDebugInfo(ze_module_handle_t hModule, zet_module_debug_info_format_t format,
                                     size_t* pSize, uint8_t* pDebugInfo,
                                     const struct hpcrun_foil_appdispatch_level0* dispatch);

// Additional Level Zero Core API wrappers
ze_result_t callZeEventPoolCreate(ze_context_handle_t hContext, const ze_event_pool_desc_t* desc,
                                 uint32_t numDevices, ze_device_handle_t* phDevices,
                                 ze_event_pool_handle_t* phEventPool,
                                 const struct hpcrun_foil_appdispatch_level0* dispatch);
ze_result_t callZeEventCreate(ze_event_pool_handle_t hEventPool, const ze_event_desc_t* desc,
                             ze_event_handle_t* phEvent,
                             const struct hpcrun_foil_appdispatch_level0* dispatch);
// Level Zero Loader API wrappers
ze_result_t callZelTracerCreate(const zel_tracer_desc_t* desc, zel_tracer_handle_t* phTracer,
                               const struct hpcrun_foil_appdispatch_level0* dispatch);
ze_result_t callZelTracerDestroy(zel_tracer_handle_t hTracer,
                                const struct hpcrun_foil_appdispatch_level0* dispatch);
ze_result_t callZelTracerSetPrologues(zel_tracer_handle_t hTracer, zel_core_callbacks_t* pCoreCbs,
                                     const struct hpcrun_foil_appdispatch_level0* dispatch);
ze_result_t callZelTracerSetEpilogues(zel_tracer_handle_t hTracer, zel_core_callbacks_t* pCoreCbs,
                                     const struct hpcrun_foil_appdispatch_level0* dispatch);
ze_result_t callZelTracerSetEnabled(zel_tracer_handle_t hTracer, ze_bool_t enable,
                                   const struct hpcrun_foil_appdispatch_level0* dispatch);

//-----------------------------------------------------------------------------
// Zebin ID map wrappers
//-----------------------------------------------------------------------------

zebin_id_map_entry_t* lookupZebinIdMap(uint32_t id);
uint32_t getZebinIdMapEntryHpctoolkitId(zebin_id_map_entry_t* entry);
void fillKernelSizeMap(zebin_id_map_entry_t* entry);
size_t lookupKernelSize(const char* kernel_name);
// Note: level0FillKernelSizeMap is internal to PC sampling library, not through API

} // namespace pcsampling

#endif // _PCSAMPLING_API_RECEIVER_HPP_
