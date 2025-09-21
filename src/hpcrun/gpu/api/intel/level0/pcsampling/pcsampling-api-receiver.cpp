// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: BSD-3-Clause

// -*-Mode: C++;-*-

//*****************************************************************************
// system includes
//*****************************************************************************

#include <cstring>
#include <cstdarg>
#include <cstdio>
#include <atomic>


//*****************************************************************************
// local includes
//*****************************************************************************

#include "../pcsampling-hpcrun-api.h"


//*****************************************************************************
// global data
//*****************************************************************************

// Global API pointer - received from libhpcrun
static pcsampling_hpcrun_api_t* hpcrun_api = nullptr;

// Initialization state
static std::atomic<bool> api_initialized(false);


//*****************************************************************************
// capabilities definition
//*****************************************************************************

static const pcsampling_capabilities_t pcsampling_capabilities = {
    .api_version = PCSAMPLING_API_VERSION,
    .supports_stall_sampling = true,
    .supports_instruction_sampling = false,  // Not yet implemented
    .max_buffer_size = 1024 * 1024  // 1MB default buffer size
};


//*****************************************************************************
// exported functions (called from libhpcrun)
//*****************************************************************************

extern "C" {

__attribute__((visibility("default")))
uint32_t
pcsampling_get_api_version(void)
{
    return PCSAMPLING_API_VERSION;
}


__attribute__((visibility("default")))
const pcsampling_capabilities_t*
pcsampling_get_capabilities(void)
{
    return &pcsampling_capabilities;
}


__attribute__((visibility("default")))
void
pcsampling_hpcrun_api_set(pcsampling_hpcrun_api_t* api)
{
    if (api && api->api_version == PCSAMPLING_API_VERSION) {
        hpcrun_api = api;
        api_initialized = true;
    }
}

} // extern "C"


//*****************************************************************************
// wrapper functions for use within PC sampling library
//*****************************************************************************

namespace pcsampling {

bool
isInitialized()
{
    return api_initialized.load() && hpcrun_api != nullptr;
}


//-----------------------------------------------------------------------------
// Memory management wrappers
//-----------------------------------------------------------------------------

gpu_activity_t*
allocActivity()
{
    if (hpcrun_api && hpcrun_api->gpu_activity_alloc) {
        return hpcrun_api->gpu_activity_alloc();
    }
    return nullptr;
}


void
freeActivity(gpu_activity_t* activity)
{
    if (hpcrun_api && hpcrun_api->gpu_activity_free) {
        hpcrun_api->gpu_activity_free(activity);
    }
}


void
initActivity(gpu_activity_t* activity)
{
    if (hpcrun_api && hpcrun_api->gpu_activity_init && activity) {
        hpcrun_api->gpu_activity_init(activity);
    }
}


gpu_activity_t**
allocActivityBatch(size_t count)
{
    if (hpcrun_api && hpcrun_api->gpu_activity_batch_alloc) {
        return hpcrun_api->gpu_activity_batch_alloc(count);
    }
    return nullptr;
}


void
freeActivityBatch(gpu_activity_t** activities, size_t count)
{
    if (hpcrun_api && hpcrun_api->gpu_activity_batch_free) {
        hpcrun_api->gpu_activity_batch_free(activities, count);
    }
}


void*
allocMemory(size_t size)
{
    if (hpcrun_api && hpcrun_api->hpcrun_malloc) {
        return hpcrun_api->hpcrun_malloc(size);
    }
    return nullptr;
}


void
freeMemory(void* ptr)
{
    if (hpcrun_api && hpcrun_api->hpcrun_free) {
        hpcrun_api->hpcrun_free(ptr);
    }
}


//-----------------------------------------------------------------------------
// Thread management wrappers
//-----------------------------------------------------------------------------

int
createProfilingThread(void* (*thread_func)(void*), void* arg, const char* name)
{
    if (hpcrun_api && hpcrun_api->create_profiling_thread) {
        return hpcrun_api->create_profiling_thread(thread_func, arg, name);
    }
    return -1;
}


void
joinProfilingThread(int thread_id)
{
    if (hpcrun_api && hpcrun_api->join_profiling_thread) {
        hpcrun_api->join_profiling_thread(thread_id);
    }
}


void
disableNewThreads()
{
    if (hpcrun_api && hpcrun_api->monitor_disable_new_threads) {
        hpcrun_api->monitor_disable_new_threads();
    }
}


void
enableNewThreads()
{
    if (hpcrun_api && hpcrun_api->monitor_enable_new_threads) {
        hpcrun_api->monitor_enable_new_threads();
    }
}


//-----------------------------------------------------------------------------
// Error handling wrappers
//-----------------------------------------------------------------------------

void
reportError(pcsampling_result_t error_code, const char* message,
           const char* file, int line)
{
    if (hpcrun_api && hpcrun_api->error_handler) {
        hpcrun_api->error_handler(error_code, message, file, line);
    }
}


void
reportWarning(const char* fmt, ...)
{
    if (hpcrun_api && hpcrun_api->warning_handler) {
        va_list args;
        va_start(args, fmt);
        // Note: This is a simplification. In production, we'd need a vprintf variant
        char buffer[1024];
        vsnprintf(buffer, sizeof(buffer), fmt, args);
        hpcrun_api->warning_handler("%s", buffer);
        va_end(args);
    }
}


//-----------------------------------------------------------------------------
// Safe entry/exit wrappers
//-----------------------------------------------------------------------------

int
safeEnter()
{
    if (hpcrun_api && hpcrun_api->safe_enter) {
        return hpcrun_api->safe_enter();
    }
    return 0;
}


void
safeExit()
{
    if (hpcrun_api && hpcrun_api->safe_exit) {
        hpcrun_api->safe_exit();
    }
}


//-----------------------------------------------------------------------------
// GPU activity channel wrappers
//-----------------------------------------------------------------------------

gpu_activity_channel_t*
lookupActivityChannel()
{
    // This gets the current thread's channel via TLS
    if (hpcrun_api && hpcrun_api->gpu_activity_channel_lookup) {
        return hpcrun_api->gpu_activity_channel_lookup();
    }
    return nullptr;
}

gpu_activity_channel_t*
lookupActivityChannel(uint32_t thread_id)
{
    // For now, we can only get the current thread's channel
    // TODO: Implement thread-specific channel lookup
    (void)thread_id; // Suppress unused parameter warning
    return lookupActivityChannel();
}


void
sendActivity(gpu_activity_channel_t* channel, gpu_activity_t* activity)
{
    if (hpcrun_api && hpcrun_api->gpu_activity_channel_send) {
        hpcrun_api->gpu_activity_channel_send(channel, activity);
    }
}

void
sendActivityDirect(uint64_t correlation_id, gpu_activity_t* activity)
{
    if (hpcrun_api && hpcrun_api->gpu_activity_send) {
        hpcrun_api->gpu_activity_send(correlation_id, activity);
    }
}


uint32_t
getThreadIdFromCorrelationId(uint64_t cid)
{
    if (hpcrun_api && hpcrun_api->gpu_activity_channel_correlation_id_get_thread_id) {
        return hpcrun_api->gpu_activity_channel_correlation_id_get_thread_id(cid);
    }
    return 0;
}


//-----------------------------------------------------------------------------
// Correlation channel wrappers
//-----------------------------------------------------------------------------

void
sendCorrelation(int device_id, uint64_t cid, gpu_activity_channel_t* channel)
{
    if (hpcrun_api && hpcrun_api->gpu_correlation_channel_send) {
        hpcrun_api->gpu_correlation_channel_send(device_id, cid, channel);
    }
}


void
receiveCorrelation(int device_id, correlation_handler_fn_t handler, void* arg)
{
    if (hpcrun_api && hpcrun_api->gpu_correlation_channel_receive) {
        hpcrun_api->gpu_correlation_channel_receive(device_id, handler, arg);
    }
}


// Alias for consistency
void
receiveCorrelationChannel(int device_id, correlation_handler_fn_t handler, void* arg)
{
    receiveCorrelation(device_id, handler, arg);
}


//-----------------------------------------------------------------------------
// Synchronization wrappers
//-----------------------------------------------------------------------------

void*
allocMutex()
{
    if (hpcrun_api && hpcrun_api->mutex_alloc) {
        return hpcrun_api->mutex_alloc();
    }
    return nullptr;
}


void
freeMutex(void* mutex)
{
    if (hpcrun_api && hpcrun_api->mutex_free) {
        hpcrun_api->mutex_free(mutex);
    }
}


void
lockMutex(void* mutex)
{
    if (hpcrun_api && hpcrun_api->mutex_lock) {
        hpcrun_api->mutex_lock(mutex);
    }
}


void
unlockMutex(void* mutex)
{
    if (hpcrun_api && hpcrun_api->mutex_unlock) {
        hpcrun_api->mutex_unlock(mutex);
    }
}


//-----------------------------------------------------------------------------
// Utility wrappers
//-----------------------------------------------------------------------------

uint64_t
nanotime()
{
    if (hpcrun_api && hpcrun_api->hpcrun_nanotime) {
        return hpcrun_api->hpcrun_nanotime();
    }
    return 0;
}


void
warn(const char* fmt, ...)
{
    if (hpcrun_api && hpcrun_api->messages_warn) {
        va_list args;
        va_start(args, fmt);
        char buffer[1024];
        vsnprintf(buffer, sizeof(buffer), fmt, args);
        hpcrun_api->messages_warn("%s", buffer);
        va_end(args);
    }
}


void
error(const char* fmt, ...)
{
    if (hpcrun_api && hpcrun_api->messages_error) {
        va_list args;
        va_start(args, fmt);
        char buffer[1024];
        vsnprintf(buffer, sizeof(buffer), fmt, args);
        hpcrun_api->messages_error("%s", buffer);
        va_end(args);
    }
}


void
trace(int level, const char* fmt, ...)
{
    if (hpcrun_api && hpcrun_api->tmsg) {
        va_list args;
        va_start(args, fmt);
        char buffer[1024];
        vsnprintf(buffer, sizeof(buffer), fmt, args);
        hpcrun_api->tmsg(level, "%s", buffer);
        va_end(args);
    }
}


const char*
getEnvironmentVariable(const char* name)
{
    if (hpcrun_api && hpcrun_api->getenv) {
        return hpcrun_api->getenv(name);
    }
    return nullptr;
}


//-----------------------------------------------------------------------------
// Level Zero function wrappers
//-----------------------------------------------------------------------------

ze_result_t
callZeDriverGet(uint32_t* pCount, ze_driver_handle_t* phDrivers,
               const struct hpcrun_foil_appdispatch_level0* dispatch)
{
    if (hpcrun_api && hpcrun_api->f_zeDriverGet) {
        return hpcrun_api->f_zeDriverGet(pCount, phDrivers, dispatch);
    }
    return ZE_RESULT_ERROR_UNINITIALIZED;
}


ze_result_t
callZeDriverGetApiVersion(ze_driver_handle_t hDriver, ze_api_version_t* version,
                         const struct hpcrun_foil_appdispatch_level0* dispatch)
{
    if (hpcrun_api && hpcrun_api->f_zeDriverGetApiVersion) {
        return hpcrun_api->f_zeDriverGetApiVersion(hDriver, version, dispatch);
    }
    return ZE_RESULT_ERROR_UNINITIALIZED;
}


ze_result_t
callZeContextCreate(ze_driver_handle_t hDriver, const ze_context_desc_t* desc,
                   ze_context_handle_t* phContext,
                   const struct hpcrun_foil_appdispatch_level0* dispatch)
{
    if (hpcrun_api && hpcrun_api->f_zeContextCreate) {
        return hpcrun_api->f_zeContextCreate(hDriver, desc, phContext, dispatch);
    }
    return ZE_RESULT_ERROR_UNINITIALIZED;
}


ze_result_t
callZeDeviceGet(ze_driver_handle_t hDriver, uint32_t* pCount,
               ze_device_handle_t* phDevices,
               const struct hpcrun_foil_appdispatch_level0* dispatch)
{
    if (hpcrun_api && hpcrun_api->f_zeDeviceGet) {
        return hpcrun_api->f_zeDeviceGet(hDriver, pCount, phDevices, dispatch);
    }
    return ZE_RESULT_ERROR_UNINITIALIZED;
}


ze_result_t
callZeDeviceGetSubDevices(ze_device_handle_t hDevice, uint32_t* pCount,
                         ze_device_handle_t* phSubdevices,
                         const struct hpcrun_foil_appdispatch_level0* dispatch)
{
    if (hpcrun_api && hpcrun_api->f_zeDeviceGetSubDevices) {
        return hpcrun_api->f_zeDeviceGetSubDevices(hDevice, pCount, phSubdevices, dispatch);
    }
    return ZE_RESULT_ERROR_UNINITIALIZED;
}


//-----------------------------------------------------------------------------
// Additional Level Zero wrapper functions - Core API
//-----------------------------------------------------------------------------

ze_result_t
callZeDeviceGetProperties(ze_device_handle_t hDevice,
                         ze_device_properties_t* pDeviceProperties,
                         const struct hpcrun_foil_appdispatch_level0* dispatch)
{
    if (hpcrun_api && hpcrun_api->f_zeDeviceGetProperties) {
        return hpcrun_api->f_zeDeviceGetProperties(hDevice, pDeviceProperties, dispatch);
    }
    return ZE_RESULT_ERROR_UNINITIALIZED;
}

ze_result_t
callZeDeviceGetRootDevice(ze_device_handle_t hDevice, ze_device_handle_t* phRootDevice,
                         const struct hpcrun_foil_appdispatch_level0* dispatch)
{
    if (hpcrun_api && hpcrun_api->f_zeDeviceGetRootDevice) {
        return hpcrun_api->f_zeDeviceGetRootDevice(hDevice, phRootDevice, dispatch);
    }
    return ZE_RESULT_ERROR_UNINITIALIZED;
}


//-----------------------------------------------------------------------------
// Level Zero Tools API wrapper functions
//-----------------------------------------------------------------------------

ze_result_t
callZetMetricGroupGet(ze_device_handle_t hDevice, uint32_t* pCount,
                     zet_metric_group_handle_t* phMetricGroups,
                     const struct hpcrun_foil_appdispatch_level0* dispatch)
{
    if (hpcrun_api && hpcrun_api->f_zetMetricGroupGet) {
        return hpcrun_api->f_zetMetricGroupGet(hDevice, pCount, phMetricGroups, dispatch);
    }
    return ZE_RESULT_ERROR_UNINITIALIZED;
}


ze_result_t
callZetMetricGroupGetProperties(zet_metric_group_handle_t hMetricGroup,
                               zet_metric_group_properties_t* pProperties,
                               const struct hpcrun_foil_appdispatch_level0* dispatch)
{
    if (hpcrun_api && hpcrun_api->f_zetMetricGroupGetProperties) {
        return hpcrun_api->f_zetMetricGroupGetProperties(hMetricGroup, pProperties, dispatch);
    }
    return ZE_RESULT_ERROR_UNINITIALIZED;
}


ze_result_t
callZetContextActivateMetricGroups(ze_context_handle_t hContext,
                                  ze_device_handle_t hDevice,
                                  uint32_t count, zet_metric_group_handle_t* phMetricGroups,
                                  const struct hpcrun_foil_appdispatch_level0* dispatch)
{
    if (hpcrun_api && hpcrun_api->f_zetContextActivateMetricGroups) {
        return hpcrun_api->f_zetContextActivateMetricGroups(hContext, hDevice, count, phMetricGroups, dispatch);
    }
    return ZE_RESULT_ERROR_UNINITIALIZED;
}


ze_result_t
callZetMetricStreamerOpen(ze_context_handle_t hContext, ze_device_handle_t hDevice,
                         zet_metric_group_handle_t hMetricGroup,
                         zet_metric_streamer_desc_t* desc, ze_event_handle_t hNotificationEvent,
                         zet_metric_streamer_handle_t* phMetricStreamer,
                         const struct hpcrun_foil_appdispatch_level0* dispatch)
{
    if (hpcrun_api && hpcrun_api->f_zetMetricStreamerOpen) {
        return hpcrun_api->f_zetMetricStreamerOpen(
            hContext, hDevice, hMetricGroup, desc, hNotificationEvent, phMetricStreamer, dispatch);
    }
    return ZE_RESULT_ERROR_UNINITIALIZED;
}


ze_result_t
callZetMetricStreamerClose(zet_metric_streamer_handle_t hMetricStreamer,
                         const struct hpcrun_foil_appdispatch_level0* dispatch)
{
    if (hpcrun_api && hpcrun_api->f_zetMetricStreamerClose) {
        return hpcrun_api->f_zetMetricStreamerClose(hMetricStreamer, dispatch);
    }
    return ZE_RESULT_ERROR_UNINITIALIZED;
}


ze_result_t
callZetMetricStreamerReadData(zet_metric_streamer_handle_t hMetricStreamer,
                             uint32_t maxReportCount, size_t* pRawDataSize,
                             uint8_t* pRawData,
                             const struct hpcrun_foil_appdispatch_level0* dispatch)
{
    if (hpcrun_api && hpcrun_api->f_zetMetricStreamerReadData) {
        return hpcrun_api->f_zetMetricStreamerReadData(hMetricStreamer, maxReportCount,
                                                        pRawDataSize, pRawData, dispatch);
    }
    return ZE_RESULT_ERROR_UNINITIALIZED;
}


ze_result_t
callZetMetricGet(zet_metric_group_handle_t hMetricGroup, uint32_t* pCount,
                zet_metric_handle_t* phMetrics,
                const struct hpcrun_foil_appdispatch_level0* dispatch)
{
    if (hpcrun_api && hpcrun_api->f_zetMetricGet) {
        return hpcrun_api->f_zetMetricGet(hMetricGroup, pCount, phMetrics, dispatch);
    }
    return ZE_RESULT_ERROR_UNINITIALIZED;
}


ze_result_t
callZetMetricGetProperties(zet_metric_handle_t hMetric,
                          zet_metric_properties_t* pProperties,
                          const struct hpcrun_foil_appdispatch_level0* dispatch)
{
    if (hpcrun_api && hpcrun_api->f_zetMetricGetProperties) {
        return hpcrun_api->f_zetMetricGetProperties(hMetric, pProperties, dispatch);
    }
    return ZE_RESULT_ERROR_UNINITIALIZED;
}


ze_result_t
callZetMetricGroupCalculateMultipleMetricValuesExp(
                          zet_metric_group_handle_t hMetricGroup,
                          zet_metric_group_calculation_type_t type, size_t rawDataSize,
                          const uint8_t* pRawData, uint32_t* pSetCount,
                          uint32_t* pTotalMetricValueCount,
                          uint32_t* pMetricCounts,
                          zet_typed_value_t* pMetricValues,
                          const struct hpcrun_foil_appdispatch_level0* dispatch)
{
    if (hpcrun_api && hpcrun_api->f_zetMetricGroupCalculateMultipleMetricValuesExp) {
        return hpcrun_api->f_zetMetricGroupCalculateMultipleMetricValuesExp(
            hMetricGroup, type, rawDataSize, pRawData, pSetCount, pTotalMetricValueCount,
            pMetricCounts, pMetricValues, dispatch);
    }
    return ZE_RESULT_ERROR_UNINITIALIZED;
}


ze_result_t
callZetModuleGetDebugInfo(ze_module_handle_t hModule, zet_module_debug_info_format_t format,
                         size_t* pSize, uint8_t* pDebugInfo,
                         const struct hpcrun_foil_appdispatch_level0* dispatch)
{
    if (hpcrun_api && hpcrun_api->f_zetModuleGetDebugInfo) {
        return hpcrun_api->f_zetModuleGetDebugInfo(hModule, format, pSize, pDebugInfo, dispatch);
    }
    return ZE_RESULT_ERROR_UNINITIALIZED;
}


//-----------------------------------------------------------------------------
// Additional Level Zero Core API wrapper functions
//-----------------------------------------------------------------------------

ze_result_t
callZeEventPoolCreate(ze_context_handle_t hContext, const ze_event_pool_desc_t* desc,
                     uint32_t numDevices, ze_device_handle_t* phDevices,
                     ze_event_pool_handle_t* phEventPool,
                     const struct hpcrun_foil_appdispatch_level0* dispatch)
{
    if (hpcrun_api && hpcrun_api->f_zeEventPoolCreate) {
        return hpcrun_api->f_zeEventPoolCreate(hContext, desc, numDevices, phDevices,
                                               phEventPool, dispatch);
    }
    return ZE_RESULT_ERROR_UNINITIALIZED;
}


ze_result_t
callZeEventCreate(ze_event_pool_handle_t hEventPool, const ze_event_desc_t* desc,
                 ze_event_handle_t* phEvent,
                 const struct hpcrun_foil_appdispatch_level0* dispatch)
{
    if (hpcrun_api && hpcrun_api->f_zeEventCreate) {
        return hpcrun_api->f_zeEventCreate(hEventPool, desc, phEvent, dispatch);
    }
    return ZE_RESULT_ERROR_UNINITIALIZED;
}


ze_result_t
callZeEventQueryStatus(ze_event_handle_t hEvent,
                      const struct hpcrun_foil_appdispatch_level0* dispatch)
{
    if (hpcrun_api && hpcrun_api->f_zeEventQueryStatus) {
        return hpcrun_api->f_zeEventQueryStatus(hEvent, dispatch);
    }
    return ZE_RESULT_ERROR_UNINITIALIZED;
}


ze_result_t
callZeEventQueryKernelTimestamp(ze_event_handle_t hEvent,
                               ze_kernel_timestamp_result_t* dstptr,
                               const struct hpcrun_foil_appdispatch_level0* dispatch)
{
    if (hpcrun_api && hpcrun_api->f_zeEventQueryKernelTimestamp) {
        return hpcrun_api->f_zeEventQueryKernelTimestamp(hEvent, dstptr, dispatch);
    }
    return ZE_RESULT_ERROR_UNINITIALIZED;
}


ze_result_t
callZeKernelGetName(ze_kernel_handle_t hKernel, size_t* pSize, char* pName,
                   const struct hpcrun_foil_appdispatch_level0* dispatch)
{
    if (hpcrun_api && hpcrun_api->f_zeKernelGetName) {
        return hpcrun_api->f_zeKernelGetName(hKernel, pSize, pName, dispatch);
    }
    return ZE_RESULT_ERROR_UNINITIALIZED;
}


ze_result_t
callZeKernelGetProperties(ze_kernel_handle_t hKernel,
                         ze_kernel_properties_t* pKernelProperties,
                         const struct hpcrun_foil_appdispatch_level0* dispatch)
{
    if (hpcrun_api && hpcrun_api->f_zeKernelGetProperties) {
        return hpcrun_api->f_zeKernelGetProperties(hKernel, pKernelProperties, dispatch);
    }
    return ZE_RESULT_ERROR_UNINITIALIZED;
}


ze_result_t
callZeModuleGetFunctionPointer(ze_module_handle_t hModule, const char* pFunctionName,
                              void** pfnFunction,
                              const struct hpcrun_foil_appdispatch_level0* dispatch)
{
    if (hpcrun_api && hpcrun_api->f_zeModuleGetFunctionPointer) {
        return hpcrun_api->f_zeModuleGetFunctionPointer(hModule, pFunctionName, pfnFunction, dispatch);
    }
    return ZE_RESULT_ERROR_UNINITIALIZED;
}


ze_result_t
callZeModuleGetKernelNames(ze_module_handle_t hModule, uint32_t* pCount,
                          const char** pNames,
                          const struct hpcrun_foil_appdispatch_level0* dispatch)
{
    if (hpcrun_api && hpcrun_api->f_zeModuleGetKernelNames) {
        return hpcrun_api->f_zeModuleGetKernelNames(hModule, pCount, pNames, dispatch);
    }
    return ZE_RESULT_ERROR_UNINITIALIZED;
}


ze_result_t
callZeCommandListGetDeviceHandle(ze_command_list_handle_t hCommandList,
                                ze_device_handle_t* phDevice,
                                const struct hpcrun_foil_appdispatch_level0* dispatch)
{
    if (hpcrun_api && hpcrun_api->f_zeCommandListGetDeviceHandle) {
        return hpcrun_api->f_zeCommandListGetDeviceHandle(hCommandList, phDevice, dispatch);
    }
    return ZE_RESULT_ERROR_UNINITIALIZED;
}


//-----------------------------------------------------------------------------
// Level Zero Loader API wrapper functions
//-----------------------------------------------------------------------------

ze_result_t
callZelTracerCreate(const zel_tracer_desc_t* desc, zel_tracer_handle_t* phTracer,
                   const struct hpcrun_foil_appdispatch_level0* dispatch)
{
    if (hpcrun_api && hpcrun_api->f_zelTracerCreate) {
        return hpcrun_api->f_zelTracerCreate(desc, phTracer, dispatch);
    }
    return ZE_RESULT_ERROR_UNINITIALIZED;
}


ze_result_t
callZelTracerDestroy(zel_tracer_handle_t hTracer,
                    const struct hpcrun_foil_appdispatch_level0* dispatch)
{
    if (hpcrun_api && hpcrun_api->f_zelTracerDestroy) {
        return hpcrun_api->f_zelTracerDestroy(hTracer, dispatch);
    }
    return ZE_RESULT_ERROR_UNINITIALIZED;
}


ze_result_t
callZelTracerSetPrologues(zel_tracer_handle_t hTracer, zel_core_callbacks_t* pCoreCbs,
                         const struct hpcrun_foil_appdispatch_level0* dispatch)
{
    if (hpcrun_api && hpcrun_api->f_zelTracerSetPrologues) {
        return hpcrun_api->f_zelTracerSetPrologues(hTracer, pCoreCbs, dispatch);
    }
    return ZE_RESULT_ERROR_UNINITIALIZED;
}


ze_result_t
callZelTracerSetEpilogues(zel_tracer_handle_t hTracer, zel_core_callbacks_t* pCoreCbs,
                         const struct hpcrun_foil_appdispatch_level0* dispatch)
{
    if (hpcrun_api && hpcrun_api->f_zelTracerSetEpilogues) {
        return hpcrun_api->f_zelTracerSetEpilogues(hTracer, pCoreCbs, dispatch);
    }
    return ZE_RESULT_ERROR_UNINITIALIZED;
}


ze_result_t
callZelTracerSetEnabled(zel_tracer_handle_t hTracer, ze_bool_t enable,
                       const struct hpcrun_foil_appdispatch_level0* dispatch)
{
    if (hpcrun_api && hpcrun_api->f_zelTracerSetEnabled) {
        return hpcrun_api->f_zelTracerSetEnabled(hTracer, enable, dispatch);
    }
    return ZE_RESULT_ERROR_UNINITIALIZED;
}


ze_result_t
callZeDriverGetExtensionFunctionAddress(ze_driver_handle_t hDriver, const char* name,
                                       void** ppFunctionAddress,
                                       const struct hpcrun_foil_appdispatch_level0* dispatch)
{
    if (hpcrun_api && hpcrun_api->f_zeDriverGetExtensionFunctionAddress) {
        return hpcrun_api->f_zeDriverGetExtensionFunctionAddress(hDriver, name, ppFunctionAddress, dispatch);
    }
    return ZE_RESULT_ERROR_UNINITIALIZED;
}


//******************************************************************************
// Zebin ID map wrapper functions
//******************************************************************************

zebin_id_map_entry_t*
lookupZebinIdMap(uint32_t id)
{
    if (hpcrun_api && hpcrun_api->zebin_id_map_lookup) {
        return hpcrun_api->zebin_id_map_lookup(id);
    }
    return nullptr;
}


uint32_t
getZebinIdMapEntryHpctoolkitId(zebin_id_map_entry_t* entry)
{
    if (hpcrun_api && hpcrun_api->zebin_id_map_entry_hpctoolkit_id_get) {
        return hpcrun_api->zebin_id_map_entry_hpctoolkit_id_get(entry);
    }
    return 0;
}


void
fillKernelSizeMap(zebin_id_map_entry_t* entry)
{
    if (hpcrun_api && hpcrun_api->level0FillKernelSizeMap) {
        // Use the API wrapper which can properly access opaque pointer fields
        hpcrun_api->level0FillKernelSizeMap(entry);
    }
}


size_t
lookupKernelSize(const char* kernel_name)
{
    if (hpcrun_api && hpcrun_api->kernel_size_lookup) {
        return hpcrun_api->kernel_size_lookup(kernel_name);
    }
    return (size_t)-1;
}

} // namespace pcsampling