// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: Apache-2.0

#ifndef _LEVEL0_PC_HPCRUN_API_H_
#define _LEVEL0_PC_HPCRUN_API_H_

//******************************************************************************
// system includes
//******************************************************************************

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// Include Level Zero headers for proper type definitions
#include <level_zero/ze_api.h>
#include <level_zero/zet_api.h>
#include <level_zero/layers/zel_tracing_api.h>

#include "../../../../../common/lean/mcs-lock.h"


//******************************************************************************
// forward declarations
//******************************************************************************

typedef struct gpu_activity_t gpu_activity_t;
typedef struct gpu_activity_channel_t gpu_activity_channel_t;
typedef struct zebin_id_map_entry_s zebin_id_map_entry_t;
struct hpcrun_foil_appdispatch_level0;


//******************************************************************************
// macros
//******************************************************************************

#define LEVEL0_PC_API_VERSION 1


//******************************************************************************
// type definitions
//******************************************************************************

// Error codes for proper error propagation
typedef enum {
    LEVEL0_PC_SUCCESS = 0,
    LEVEL0_PC_ERROR_INIT_FAILED = -1,
    LEVEL0_PC_ERROR_LIBRARY_LOAD = -2,
    LEVEL0_PC_ERROR_LEVEL0_API = -3,
    LEVEL0_PC_ERROR_RESOURCE_EXHAUSTED = -4,
    LEVEL0_PC_ERROR_VERSION_MISMATCH = -5,
    LEVEL0_PC_ERROR_NOT_SUPPORTED = -6
} level0_pc_result_t;

// Capabilities for feature negotiation
typedef struct level0_pc_capabilities_t {
    uint32_t api_version;
    bool supports_stall_sampling;
    bool supports_instruction_sampling;
    size_t max_buffer_size;
} level0_pc_capabilities_t;

// Function pointer types for cleaner API definition
typedef void (*level0_pc_error_handler_t)(int error_code, const char* message,
                                          const char* file, int line);
typedef void (*level0_pc_warning_handler_t)(const char* fmt, ...);
typedef void (*level0_pc_cleanup_handler_t)(void* arg);
typedef void (*correlation_handler_fn_t)(uint64_t, gpu_activity_channel_t*, void*);

// API structure for functions PC sampling needs from libhpcrun
typedef struct level0_pc_hpcrun_api_t {
    // API version for compatibility checking
    uint32_t api_version;

    // Memory management - CRITICAL for cross-library safety
    gpu_activity_t* (*gpu_activity_alloc)(void);
    void (*gpu_activity_free)(gpu_activity_t* activity);
    gpu_activity_t** (*gpu_activity_batch_alloc)(size_t count);
    void (*gpu_activity_batch_free)(gpu_activity_t** activities, size_t count);
    void (*gpu_activity_init)(gpu_activity_t* activity);

    // General memory allocation (for thread args, etc.)
    void* (*hpcrun_malloc)(size_t size);
    void (*hpcrun_free)(void* ptr);

    // Thread management - Must use hpcrun's thread infrastructure
    int (*create_profiling_thread)(void* (*thread_func)(void*), void* arg, const char* name);
    void (*join_profiling_thread)(int thread_id);

    // sample period
    uint64_t (*get_sample_period)();

    // Error handling - No exit() calls allowed
    level0_pc_error_handler_t error_handler;
    level0_pc_warning_handler_t warning_handler;

    // Safe entry/exit
    int (*safe_enter)(void);
    void (*safe_exit)(void);

    // GPU activity channel operations
    gpu_activity_channel_t* (*gpu_activity_channel_lookup)(void);
    void (*gpu_activity_channel_send)(gpu_activity_channel_t* channel, const gpu_activity_t* activity);
    uint32_t (*gpu_activity_channel_correlation_id_get_thread_id)(uint64_t correlation_id);

    // Direct activity send using correlation ID (for PC sampling thread)
    void (*gpu_activity_send)(uint64_t correlation_id, gpu_activity_t* activity);

    // Correlation channel operations
    void (*gpu_correlation_channel_send)(uint64_t idx, uint64_t host_correlation_id,
                                        gpu_activity_channel_t* channel);
    void (*gpu_correlation_channel_receive)(uint64_t idx,
                                           void (*receive_fn)(uint64_t, gpu_activity_channel_t*, void*), void* data);

    // Thread monitoring
    void (*monitor_disable_new_threads)(void);
    void (*monitor_enable_new_threads)(void);

    // Synchronization primitives
    void* (*mutex_alloc)(void);
    void (*mutex_free)(void* mutex);
    void (*mutex_lock)(void* mutex);
    void (*mutex_unlock)(void* mutex);

    // MCS lock helpers
    void (*mcs_lock)(mcs_lock_t* lock, mcs_node_t* node);
    void (*mcs_unlock)(mcs_lock_t* lock, mcs_node_t* node);

    // Other utilities
    uint64_t (*hpcrun_nanotime)(void);
    void (*messages_warn)(const char* fmt, ...);
    void (*messages_error)(const char* fmt, ...);
    void (*tmsg)(int level, const char* fmt, ...);

    // Environment variable access
    const char* (*getenv)(const char* name);

    // Hashing utilities
    int (*crypto_compute_hash_string)(const void* data, size_t size,
                                      char* out, unsigned int out_len);

    // Cleanup registration
    int (*register_cleanup_handler)(level0_pc_cleanup_handler_t handler, void* arg);

    // Zebin ID map operations
    zebin_id_map_entry_t* (*zebin_id_map_lookup)(uint32_t id);
    uint32_t (*zebin_id_map_entry_hpctoolkit_id_get)(zebin_id_map_entry_t* entry);
    void (*level0FillKernelSizeMap)(zebin_id_map_entry_t* entry);
    size_t (*kernel_size_lookup)(const char* kernel_name);

    // Level Zero Core API functions (function pointers for dynamic dispatch)
    ze_result_t (*f_zeDriverGet)(uint32_t* pCount, ze_driver_handle_t* phDrivers,
                                 const struct hpcrun_foil_appdispatch_level0* dispatch);
    ze_result_t (*f_zeDriverGetApiVersion)(ze_driver_handle_t hDriver, ze_api_version_t* version,
                                          const struct hpcrun_foil_appdispatch_level0* dispatch);
    ze_result_t (*f_zeContextCreate)(ze_driver_handle_t hDriver, const ze_context_desc_t* desc,
                                     ze_context_handle_t* phContext,
                                     const struct hpcrun_foil_appdispatch_level0* dispatch);
    ze_result_t (*f_zeContextDestroy)(ze_context_handle_t hContext,
                                      const struct hpcrun_foil_appdispatch_level0* dispatch);
    ze_result_t (*f_zeContextMakeMemoryResident)(ze_context_handle_t hContext, ze_device_handle_t hDevice,
                                                 void* ptr, size_t size,
                                                 const struct hpcrun_foil_appdispatch_level0* dispatch);
    ze_result_t (*f_zeContextEvictMemory)(ze_context_handle_t hContext, ze_device_handle_t hDevice,
                                          void* ptr, size_t size,
                                          const struct hpcrun_foil_appdispatch_level0* dispatch);
    ze_result_t (*f_zeDeviceGet)(ze_driver_handle_t hDriver, uint32_t* pCount,
                                 ze_device_handle_t* phDevices,
                                 const struct hpcrun_foil_appdispatch_level0* dispatch);
    ze_result_t (*f_zeDeviceGetSubDevices)(ze_device_handle_t hDevice, uint32_t* pCount,
                                           ze_device_handle_t* phSubdevices,
                                           const struct hpcrun_foil_appdispatch_level0* dispatch);
    ze_result_t (*f_zeDeviceGetProperties)(ze_device_handle_t hDevice,
                                           ze_device_properties_t* pDeviceProperties,
                                           const struct hpcrun_foil_appdispatch_level0* dispatch);
    ze_result_t (*f_zeDeviceGetRootDevice)(ze_device_handle_t hDevice,
                                           ze_device_handle_t* phRootDevice,
                                           const struct hpcrun_foil_appdispatch_level0* dispatch);
    ze_result_t (*f_zeDriverGetExtensionFunctionAddress)(ze_driver_handle_t hDriver, const char* name,
                                                         void** ppFunctionAddress,
                                                         const struct hpcrun_foil_appdispatch_level0* dispatch);
    ze_result_t (*f_zeEventCreate)(ze_event_pool_handle_t hEventPool, const ze_event_desc_t* desc,
                                   ze_event_handle_t* phEvent,
                                   const struct hpcrun_foil_appdispatch_level0* dispatch);
    ze_result_t (*f_zeEventDestroy)(ze_event_handle_t hEvent,
                                    const struct hpcrun_foil_appdispatch_level0* dispatch);
    ze_result_t (*f_zeEventPoolCreate)(ze_context_handle_t hContext, const ze_event_pool_desc_t* desc,
                                       uint32_t numDevices, ze_device_handle_t* phDevices,
                                       ze_event_pool_handle_t* phEventPool,
                                       const struct hpcrun_foil_appdispatch_level0* dispatch);
    ze_result_t (*f_zeEventPoolDestroy)(ze_event_pool_handle_t hEventPool,
                                        const struct hpcrun_foil_appdispatch_level0* dispatch);
    ze_result_t (*f_zeEventHostSynchronize)(ze_event_handle_t hEvent, uint64_t timeout,
                                            const struct hpcrun_foil_appdispatch_level0* dispatch);
    ze_result_t (*f_zeEventQueryStatus)(ze_event_handle_t hEvent,
                                        const struct hpcrun_foil_appdispatch_level0* dispatch);
    ze_result_t (*f_zeEventQueryKernelTimestamp)(ze_event_handle_t hEvent,
                                                 ze_kernel_timestamp_result_t* dstptr,
                                                 const struct hpcrun_foil_appdispatch_level0* dispatch);
    ze_result_t (*f_zeKernelCreate)(ze_module_handle_t hModule, const ze_kernel_desc_t* desc,
                                    ze_kernel_handle_t* phKernel,
                                    const struct hpcrun_foil_appdispatch_level0* dispatch);
    ze_result_t (*f_zeKernelDestroy)(ze_kernel_handle_t hKernel,
                                     const struct hpcrun_foil_appdispatch_level0* dispatch);
    ze_result_t (*f_zeKernelGetName)(ze_kernel_handle_t hKernel, size_t* pSize, char* pName,
                                     const struct hpcrun_foil_appdispatch_level0* dispatch);
    ze_result_t (*f_zeKernelGetProperties)(ze_kernel_handle_t hKernel,
                                           ze_kernel_properties_t* pKernelProperties,
                                           const struct hpcrun_foil_appdispatch_level0* dispatch);
    ze_result_t (*f_zeModuleCreate)(ze_context_handle_t hContext, ze_device_handle_t hDevice,
                                    const ze_module_desc_t* desc, ze_module_handle_t* phModule,
                                    ze_module_build_log_handle_t* phBuildLog,
                                    const struct hpcrun_foil_appdispatch_level0* dispatch);
    ze_result_t (*f_zeModuleDestroy)(ze_module_handle_t hModule,
                                     const struct hpcrun_foil_appdispatch_level0* dispatch);
    ze_result_t (*f_zeModuleGetFunctionPointer)(ze_module_handle_t hModule, const char* pFunctionName,
                                               void** pfnFunction,
                                               const struct hpcrun_foil_appdispatch_level0* dispatch);
    ze_result_t (*f_zeModuleGetKernelNames)(ze_module_handle_t hModule, uint32_t* pCount,
                                            const char** pNames,
                                            const struct hpcrun_foil_appdispatch_level0* dispatch);
    ze_result_t (*f_zeCommandListCreate)(ze_context_handle_t hContext, ze_device_handle_t hDevice,
                                         const ze_command_list_desc_t* desc, ze_command_list_handle_t* phCommandList,
                                         const struct hpcrun_foil_appdispatch_level0* dispatch);
    ze_result_t (*f_zeCommandListDestroy)(ze_command_list_handle_t hCommandList,
                                          const struct hpcrun_foil_appdispatch_level0* dispatch);
    ze_result_t (*f_zeCommandListReset)(ze_command_list_handle_t hCommandList,
                                        const struct hpcrun_foil_appdispatch_level0* dispatch);
    ze_result_t (*f_zeCommandListClose)(ze_command_list_handle_t hCommandList,
                                        const struct hpcrun_foil_appdispatch_level0* dispatch);
    ze_result_t (*f_zeCommandListGetCommandQueueGroupOrdinal)(ze_command_list_handle_t hCommandList,
                                                              uint32_t* pOrdinal,
                                                              const struct hpcrun_foil_appdispatch_level0* dispatch);
    ze_result_t (*f_zeCommandListGetDeviceHandle)(ze_command_list_handle_t hCommandList,
                                                 ze_device_handle_t* phDevice,
                                                 const struct hpcrun_foil_appdispatch_level0* dispatch);
    ze_result_t (*f_zeCommandListHostSynchronize)(ze_command_list_handle_t hCommandList, uint64_t timeout,
                                                 const struct hpcrun_foil_appdispatch_level0* dispatch);
    ze_result_t (*f_zeCommandListAppendBarrier)(ze_command_list_handle_t hCommandList, ze_event_handle_t hSignalEvent,
                                               uint32_t numWaitEvents, ze_event_handle_t* phWaitEvents,
                                               const struct hpcrun_foil_appdispatch_level0* dispatch);

    // Level Zero Tools API functions
    ze_result_t (*f_zetContextActivateMetricGroups)(zet_context_handle_t hContext, ze_device_handle_t hDevice,
                                                    uint32_t count, zet_metric_group_handle_t* phMetricGroups,
                                                    const struct hpcrun_foil_appdispatch_level0* dispatch);
    ze_result_t (*f_zetMetricGet)(zet_metric_group_handle_t hMetricGroup, uint32_t* pCount,
                                  zet_metric_handle_t* phMetrics,
                                  const struct hpcrun_foil_appdispatch_level0* dispatch);
    ze_result_t (*f_zetMetricGetProperties)(zet_metric_handle_t hMetric,
                                            zet_metric_properties_t* pProperties,
                                            const struct hpcrun_foil_appdispatch_level0* dispatch);
    ze_result_t (*f_zetMetricGroupCalculateMultipleMetricValuesExp)(
                                            zet_metric_group_handle_t hMetricGroup,
                                            zet_metric_group_calculation_type_t type, size_t rawDataSize,
                                            const uint8_t* pRawData, uint32_t* pSetCount,
                                            uint32_t* pTotalMetricValueCount,
                                            uint32_t* pMetricCounts,
                                            zet_typed_value_t* pMetricValues,
                                            const struct hpcrun_foil_appdispatch_level0* dispatch);
    ze_result_t (*f_zetMetricGroupGet)(ze_device_handle_t hDevice, uint32_t* pCount,
                                       zet_metric_group_handle_t* phMetricGroups,
                                       const struct hpcrun_foil_appdispatch_level0* dispatch);
    ze_result_t (*f_zetMetricGroupGetProperties)(zet_metric_group_handle_t hMetricGroup,
                                                 zet_metric_group_properties_t* pProperties,
                                                 const struct hpcrun_foil_appdispatch_level0* dispatch);
    ze_result_t (*f_zetMetricStreamerClose)(zet_metric_streamer_handle_t hMetricStreamer,
                                            const struct hpcrun_foil_appdispatch_level0* dispatch);
    ze_result_t (*f_zetMetricStreamerOpen)(ze_context_handle_t hContext, ze_device_handle_t hDevice,
                                           zet_metric_group_handle_t hMetricGroup,
                                           zet_metric_streamer_desc_t* desc, ze_event_handle_t hNotificationEvent,
                                           zet_metric_streamer_handle_t* phMetricStreamer,
                                           const struct hpcrun_foil_appdispatch_level0* dispatch);
    ze_result_t (*f_zetMetricStreamerReadData)(zet_metric_streamer_handle_t hMetricStreamer,
                                               uint32_t maxReportCount, size_t* pRawDataSize,
                                               uint8_t* pRawData,
                                               const struct hpcrun_foil_appdispatch_level0* dispatch);
    ze_result_t (*f_zetModuleGetDebugInfo)(ze_module_handle_t hModule, zet_module_debug_info_format_t format,
                                           size_t* pSize, uint8_t* pDebugInfo,
                                           const struct hpcrun_foil_appdispatch_level0* dispatch);

    // Level Zero Loader API functions (zel_*)
    ze_result_t (*f_zelTracerCreate)(const zel_tracer_desc_t* desc, zel_tracer_handle_t* phTracer,
                                     const struct hpcrun_foil_appdispatch_level0* dispatch);
    ze_result_t (*f_zelTracerDestroy)(zel_tracer_handle_t hTracer,
                                      const struct hpcrun_foil_appdispatch_level0* dispatch);
    ze_result_t (*f_zelTracerSetPrologues)(zel_tracer_handle_t hTracer, zel_core_callbacks_t* pCoreCbs,
                                           const struct hpcrun_foil_appdispatch_level0* dispatch);
    ze_result_t (*f_zelTracerSetEpilogues)(zel_tracer_handle_t hTracer, zel_core_callbacks_t* pCoreCbs,
                                           const struct hpcrun_foil_appdispatch_level0* dispatch);
    ze_result_t (*f_zelTracerSetEnabled)(zel_tracer_handle_t hTracer, ze_bool_t enable,
                                         const struct hpcrun_foil_appdispatch_level0* dispatch);
} level0_pc_hpcrun_api_t;


//******************************************************************************
// interface functions exported by PC sampling library
//******************************************************************************

#ifdef __cplusplus
extern "C" {
#endif

// Version and capability checking
uint32_t level0_pc_get_api_version(void);
const level0_pc_capabilities_t* level0_pc_get_capabilities(void);

// Set the API table from hpcrun
void level0_pc_hpcrun_api_set(level0_pc_hpcrun_api_t* api);

// Initialization with error propagation
level0_pc_result_t level0_pc_init(
    const struct hpcrun_foil_appdispatch_level0* dispatch,
    char* error_buffer,
    size_t error_buffer_size
);

level0_pc_result_t level0_pc_shutdown(void);
bool level0_pc_enabled(void);

// Context-based profiler creation
void* level0_pc_profiler_create(
    const struct hpcrun_foil_appdispatch_level0* dispatch,
    level0_pc_result_t* result
);

void level0_pc_profiler_destroy(void* profiler);

// Correlation ID management
void level0_pc_update_correlation_id(uint64_t cid, gpu_activity_channel_t* channel, void* context);

#ifdef __cplusplus
}
#endif

#endif // _LEVEL0_PC_HPCRUN_API_H_
