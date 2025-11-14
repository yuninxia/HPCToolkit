// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: Apache-2.0

//******************************************************************************
// system includes
//******************************************************************************

#define _GNU_SOURCE  // for dlmopen

#include <pthread.h>
#include <dlfcn.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>


//******************************************************************************
// local includes
//******************************************************************************

#include "level0-pc-shim.h"
#include "../hpcrun-sonames.h"
#include "../../../../messages/messages.h"
#include "../../../../safe-sampling.h"
#include "../../../../libmonitor/monitor.h"
#include "../../../../memory/hpcrun-malloc.h"
#include "../../../activity/gpu-activity.h"
#include "../../../activity/gpu-activity-channel.h"
#include "../../../activity/gpu-activity-send.h"
#include "../../../common/gpu-monitoring.h"
#include "../../../activity/correlation/gpu-correlation-channel.h"
#include "../../../../utilities/hpcrun-nanotime.h"
#include "../../../../../common/lean/mcs-lock.h"
#include "../../../../../common/lean/crypto-hash.h"
#include "level0-id-map.h"
#include "../../../../foil/level0.h"  // for Level Zero function pointers


//******************************************************************************
// local data
//******************************************************************************

static pthread_once_t once_control = PTHREAD_ONCE_INIT;
static level0_pc_result_t init_result = LEVEL0_PC_ERROR_INIT_FAILED;
static void* level0_pc_lib_handle = NULL;

static void
messages_error_wrapper(const char* fmt, ...)
{
    if (!fmt) return;

    char buffer[1024];
    va_list args;
    va_start(args, fmt);
    int written = vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    if (written < 0) return;

    if ((size_t)written >= sizeof(buffer)) {
        static const char suffix[] = " ...";
        size_t suffix_len = sizeof(suffix) - 1;
        if (suffix_len < sizeof(buffer)) {
            memcpy(buffer + sizeof(buffer) - suffix_len - 1, suffix, suffix_len);
            buffer[sizeof(buffer) - 1] = '\0';
        }
    }

    EEMSG("%s", buffer);
}

static ze_result_t
zeDriverGetExtensionFunctionAddress_wrapper(ze_driver_handle_t hDriver, const char* name,
                                            void** ppFunctionAddress,
                                            const struct hpcrun_foil_appdispatch_level0* dispatch)
{
    return f_zeDriverGetExtensionFunctionAddress(hDriver, name, ppFunctionAddress, dispatch);
}

// Function pointers to PC sampling library
static uint32_t (*level0_pc_get_api_version_fn)(void);
static const level0_pc_capabilities_t* (*level0_pc_get_capabilities_fn)(void);
static void (*level0_pc_hpcrun_api_set_fn)(level0_pc_hpcrun_api_t*);
static level0_pc_result_t (*level0_pc_init_fn)(const struct hpcrun_foil_appdispatch_level0*, char*, size_t);
static level0_pc_result_t (*level0_pc_shutdown_fn)(void);
static bool (*level0_pc_enabled_fn)(void);
static void* (*level0_pc_profiler_create_fn)(const struct hpcrun_foil_appdispatch_level0*, level0_pc_result_t*);
static void (*level0_pc_profiler_destroy_fn)(void*);
static void (*level0_pc_update_correlation_id_fn)(uint64_t, gpu_activity_channel_t*, void*);


//******************************************************************************
// thread management
//******************************************************************************

typedef struct thread_registry_entry {
    pthread_t thread;
    int id;
    char name[64];
    struct thread_registry_entry* next;
} thread_registry_entry;

static thread_registry_entry* thread_registry = NULL;
static int next_thread_id = 1;
static pthread_mutex_t thread_registry_mutex = PTHREAD_MUTEX_INITIALIZER;


//******************************************************************************
// helper functions
//******************************************************************************

static gpu_activity_t*
gpu_activity_alloc_wrapper(void)
{
    // Use hpcrun's allocator instead of malloc
    gpu_activity_t* activity = (gpu_activity_t*)hpcrun_malloc(sizeof(gpu_activity_t));
    if (activity) {
        memset(activity, 0, sizeof(gpu_activity_t));
    }
    return activity;
}


static void
gpu_activity_free_wrapper(gpu_activity_t* activity)
{
    // HPCToolkit uses memory pools, individual items cannot be freed
    // Memory is reclaimed when the entire pool is reclaimed
    (void)activity;
}


static gpu_activity_t**
gpu_activity_batch_alloc_wrapper(size_t count)
{
    if (count == 0) return NULL;

    gpu_activity_t** batch = (gpu_activity_t**)hpcrun_malloc(count * sizeof(gpu_activity_t*));
    if (batch) {
        for (size_t i = 0; i < count; i++) {
            batch[i] = gpu_activity_alloc_wrapper();
            if (!batch[i]) {
                // Clean up on failure
                // Cleanup not needed - HPCToolkit uses memory pools
                (void)batch;
                return NULL;
            }
        }
    }
    return batch;
}


static void
gpu_activity_batch_free_wrapper(gpu_activity_t** activities, size_t count)
{
    if (!activities) return;

    // HPCToolkit uses memory pools, no individual free
    (void)activities;
    (void)count;
}


static void
error_handler_wrapper(int error_code, const char* message, const char* file, int line)
{
    EEMSG("PC Sampling Error [%d] at %s:%d: %s", error_code, file, line, message);
}


static int
create_profiling_thread_wrapper(void* (*func)(void*), void* arg, const char* name)
{
    monitor_disable_new_threads();

    pthread_mutex_lock(&thread_registry_mutex);

    thread_registry_entry* entry = (thread_registry_entry*)hpcrun_malloc(sizeof(thread_registry_entry));
    if (!entry) {
        pthread_mutex_unlock(&thread_registry_mutex);
        monitor_enable_new_threads();
        return -1;
    }

    int ret = pthread_create(&entry->thread, NULL, func, arg);
    if (ret == 0) {
        entry->id = next_thread_id++;
        strncpy(entry->name, name ? name : "level0_pc", sizeof(entry->name) - 1);
        entry->name[sizeof(entry->name) - 1] = '\0';
        entry->next = thread_registry;
        thread_registry = entry;

        pthread_mutex_unlock(&thread_registry_mutex);
        monitor_enable_new_threads();

        return entry->id;
    }

    // Note: hpcrun memory cannot be freed according to the API documentation
    // The allocated memory will be reclaimed when the profiler shuts down
    pthread_mutex_unlock(&thread_registry_mutex);
    monitor_enable_new_threads();
    return -1;
}


static gpu_activity_channel_t*
gpu_activity_channel_lookup_wrapper(void)
{
    // Get current thread's ID and lookup its channel
    // This is a workaround for the API mismatch
    // TODO: This may not work correctly for cross-thread lookups
    pthread_t self = pthread_self();
    uint32_t thread_id = (uint32_t)(uintptr_t)self;  // Simple hash of pthread_t
    return gpu_activity_channel_lookup(thread_id);
}

static void
join_profiling_thread_wrapper(int thread_id)
{
    pthread_mutex_lock(&thread_registry_mutex);

    thread_registry_entry* prev = NULL;
    thread_registry_entry* curr = thread_registry;

    while (curr) {
        if (curr->id == thread_id) {
            pthread_mutex_unlock(&thread_registry_mutex);
            pthread_join(curr->thread, NULL);
            pthread_mutex_lock(&thread_registry_mutex);

            // Remove from registry
            if (prev) {
                prev->next = curr->next;
            } else {
                thread_registry = curr->next;
            }

            // Memory from hpcrun_malloc cannot be freed individually
            (void)curr;
            pthread_mutex_unlock(&thread_registry_mutex);
            return;
        }
        prev = curr;
        curr = curr->next;
    }

    pthread_mutex_unlock(&thread_registry_mutex);
    EMSG("Thread ID %d not found in registry", thread_id);
}


static void*
mutex_alloc_wrapper(void)
{
    pthread_mutex_t* mutex = (pthread_mutex_t*)hpcrun_malloc(sizeof(pthread_mutex_t));
    if (mutex) {
        pthread_mutex_init(mutex, NULL);
    }
    return mutex;
}


static void
mutex_free_wrapper(void* mutex)
{
    if (mutex) {
        pthread_mutex_destroy((pthread_mutex_t*)mutex);
        // Memory from hpcrun_malloc cannot be freed
    }
}


// Wrapper function to fill kernel size map with proper access to opaque zebin_id_map_entry_t
// This function can access the internal structure fields that the PC sampling library cannot
// Note: We cannot directly call into the PC sampling library from here since it's loaded dynamically
// Instead, we'll just validate the entry has valid data and let the PC sampling library handle it
static void
level0_fill_kernel_size_map_wrapper(zebin_id_map_entry_t* entry)
{
    if (entry == NULL) {
        EMSG("[WARNING] Null entry passed to level0_fill_kernel_size_map_wrapper");
        return;
    }

    // Get the ELF vector through the proper API function
    SymbolVector* symbols = zebin_id_map_entry_elf_vector_get(entry);
    if (symbols == NULL) {
        EMSG("[WARNING] Null symbol vector in entry passed to level0_fill_kernel_size_map_wrapper");
        return;
    }

    if (symbols->nsymbols <= 0) {
        EMSG("[WARNING] No symbols found in entry passed to level0_fill_kernel_size_map_wrapper");
        return;
    }

    // The PC sampling library cannot use this wrapper effectively because it would create
    // a circular dependency. The real fix is to not call level0FillKernelSizeMap at all
    // from the PC sampling library when the entry is opaque.

    // For now, just log that we have valid symbol data but can't process it
    TMSG(LEVEL0, "Found %d symbols in zebin entry but cannot process due to library separation", symbols->nsymbols);
}


static void
mutex_lock_wrapper(void* mutex)
{
    if (mutex) {
        pthread_mutex_lock((pthread_mutex_t*)mutex);
    }
}


static void
mutex_unlock_wrapper(void* mutex)
{
    if (mutex) {
        pthread_mutex_unlock((pthread_mutex_t*)mutex);
    }
}


//******************************************************************************
// API structure
//******************************************************************************

static const char*
hpcrun_getenv_wrapper(const char* name)
{
  return getenv(name);
}


static level0_pc_hpcrun_api_t level0_pc_hpcrun_api = {
    .api_version = LEVEL0_PC_API_VERSION,

    // Memory management
    .gpu_activity_alloc = gpu_activity_alloc_wrapper,
    .gpu_activity_free = gpu_activity_free_wrapper,
    .gpu_activity_batch_alloc = gpu_activity_batch_alloc_wrapper,
    .gpu_activity_batch_free = gpu_activity_batch_free_wrapper,
    .gpu_activity_init = gpu_activity_init,
    .hpcrun_malloc = hpcrun_malloc,
    .hpcrun_free = NULL,  // HPCToolkit doesn't support individual free

    // Thread management
    .create_profiling_thread = create_profiling_thread_wrapper,
    .join_profiling_thread = join_profiling_thread_wrapper,

    .get_sample_period = gpu_monitoring_instruction_sampling_period_get,

    // Error handling
    .error_handler = error_handler_wrapper,
    .warning_handler = (level0_pc_warning_handler_t)EMSG,

    // Safe entry/exit
    .safe_enter = hpcrun_safe_enter_noinline,
    .safe_exit = hpcrun_safe_exit_noinline,

    // GPU activity channel operations
    .gpu_activity_channel_lookup = gpu_activity_channel_lookup_wrapper,
    .gpu_activity_channel_send = gpu_activity_channel_send,
    .gpu_activity_channel_correlation_id_get_thread_id = gpu_activity_channel_correlation_id_get_thread_id,

    // Direct activity send using correlation ID
    .gpu_activity_send = gpu_activity_send,

    // Correlation channel operations
    .gpu_correlation_channel_send = gpu_correlation_channel_send,
    .gpu_correlation_channel_receive = gpu_correlation_channel_receive,

    // Thread monitoring
    .monitor_disable_new_threads = monitor_disable_new_threads,
    .monitor_enable_new_threads = monitor_enable_new_threads,

    // Synchronization primitives
    .mutex_alloc = mutex_alloc_wrapper,
    .mutex_free = mutex_free_wrapper,
    .mutex_lock = mutex_lock_wrapper,
    .mutex_unlock = mutex_unlock_wrapper,
    .mcs_lock = mcs_lock,
    .mcs_unlock = mcs_unlock,

    // Other utilities
    .hpcrun_nanotime = hpcrun_nanotime,
    .messages_warn = (void (*)(const char*, ...))EMSG,
    .messages_error = messages_error_wrapper,
    .tmsg = NULL,
    .getenv = hpcrun_getenv_wrapper,
    .crypto_compute_hash_string = crypto_compute_hash_string,

    // Cleanup registration
    .register_cleanup_handler = NULL,  // TODO: implement if needed

    // Zebin ID map functions
    .zebin_id_map_lookup = zebin_id_map_lookup,
    .zebin_id_map_entry_hpctoolkit_id_get = zebin_id_map_entry_hpctoolkit_id_get,
    .level0FillKernelSizeMap = level0_fill_kernel_size_map_wrapper,  // Wrapper to access opaque pointer fields
    .kernel_size_lookup = NULL,

    // Level Zero functions - pass through to the real implementations
    // Core API functions
    .f_zeDriverGet = f_zeDriverGet,
    .f_zeDriverGetApiVersion = f_zeDriverGetApiVersion,
    .f_zeContextCreate = f_zeContextCreate,
    .f_zeDeviceGet = f_zeDeviceGet,
    .f_zeDeviceGetSubDevices = f_zeDeviceGetSubDevices,
    .f_zeDeviceGetProperties = f_zeDeviceGetProperties,
    .f_zeDeviceGetRootDevice = f_zeDeviceGetRootDevice,
    .f_zeEventCreate = f_zeEventCreate,
    .f_zeEventPoolCreate = f_zeEventPoolCreate,
    .f_zeEventQueryStatus = f_zeEventQueryStatus,
    .f_zeEventQueryKernelTimestamp = f_zeEventQueryKernelTimestamp,
    .f_zeKernelGetName = f_zeKernelGetName,
    .f_zeKernelGetProperties = f_zeKernelGetProperties,
    .f_zeModuleGetFunctionPointer = f_zeModuleGetFunctionPointer,
    .f_zeModuleGetKernelNames = f_zeModuleGetKernelNames,
    .f_zeCommandListGetDeviceHandle = f_zeCommandListGetDeviceHandle,
    .f_zeDriverGetExtensionFunctionAddress = zeDriverGetExtensionFunctionAddress_wrapper,

    // Tools API functions
    .f_zetContextActivateMetricGroups = f_zetContextActivateMetricGroups,
    .f_zetMetricGet = f_zetMetricGet,
    .f_zetMetricGetProperties = f_zetMetricGetProperties,
    .f_zetMetricGroupCalculateMultipleMetricValuesExp = f_zetMetricGroupCalculateMultipleMetricValuesExp,
    .f_zetMetricGroupGet = f_zetMetricGroupGet,
    .f_zetMetricGroupGetProperties = f_zetMetricGroupGetProperties,
    .f_zetMetricStreamerClose = f_zetMetricStreamerClose,
    .f_zetMetricStreamerOpen = f_zetMetricStreamerOpen,
    .f_zetMetricStreamerReadData = f_zetMetricStreamerReadData,
    .f_zetModuleGetDebugInfo = f_zetModuleGetDebugInfo,

    // Level Zero Loader API functions (zel_*)
    .f_zelTracerCreate = f_zelTracerCreate,
    .f_zelTracerDestroy = f_zelTracerDestroy,
    .f_zelTracerSetPrologues = f_zelTracerSetPrologues,
    .f_zelTracerSetEpilogues = f_zelTracerSetEpilogues,
    .f_zelTracerSetEnabled = f_zelTracerSetEnabled
};


//******************************************************************************
// initialization
//******************************************************************************

static void
init(void)
{
    TMSG(LEVEL0, "PC Sampling: Loading dynamic library");

    // Note: EEMSG and TMSG are macros that can't be used as function pointers
    // These will be set to NULL and PC sampling library should use its own logging
    level0_pc_hpcrun_api.messages_error = messages_error_wrapper;
    level0_pc_hpcrun_api.tmsg = NULL;

    // Determine namespace scope based on environment variable
    // Following GTPin pattern: default to isolation (LM_ID_NEWLM)
    // Use shared namespace (LM_ID_BASE) only when explicitly requested
    Lmid_t scope = getenv("HPCRUN_LEVEL0_PC_VISIBLE") ? LM_ID_BASE : LM_ID_NEWLM;

    // Load PC sampling library
    level0_pc_lib_handle = dlmopen(scope, HPCRUN_LEVEL0_PC_CXX_SO, RTLD_LOCAL | RTLD_LAZY);

    if (level0_pc_lib_handle == NULL) {
        EEMSG("Unable to load PC sampling library %s: %s", HPCRUN_LEVEL0_PC_CXX_SO, dlerror());
        init_result = LEVEL0_PC_ERROR_LIBRARY_LOAD;
        return;
    }

    // Check API version first
    level0_pc_get_api_version_fn = dlsym(level0_pc_lib_handle, "level0_pc_get_api_version");
    if (!level0_pc_get_api_version_fn) {
        EEMSG("PC sampling library missing level0_pc_get_api_version function");
        init_result = LEVEL0_PC_ERROR_VERSION_MISMATCH;
        dlclose(level0_pc_lib_handle);
        level0_pc_lib_handle = NULL;
        return;
    }

    uint32_t lib_version = level0_pc_get_api_version_fn();
    if (lib_version != LEVEL0_PC_API_VERSION) {
        EEMSG("PC sampling library API version mismatch: expected %d, got %d",
              LEVEL0_PC_API_VERSION, lib_version);
        init_result = LEVEL0_PC_ERROR_VERSION_MISMATCH;
        dlclose(level0_pc_lib_handle);
        level0_pc_lib_handle = NULL;
        return;
    }

    // Get capabilities and check if required features are supported
    level0_pc_get_capabilities_fn = dlsym(level0_pc_lib_handle, "level0_pc_get_capabilities");
    if (level0_pc_get_capabilities_fn) {
        const level0_pc_capabilities_t* caps = level0_pc_get_capabilities_fn();
        if (getenv("HPCRUN_LEVEL0_PC_STALL") && !caps->supports_stall_sampling) {
            EMSG("Stall sampling requested but not supported by PC sampling library");
        }
        TMSG(LEVEL0, "PC Sampling capabilities: stall=%d, instruction=%d, max_buffer=%zu",
             caps->supports_stall_sampling, caps->supports_instruction_sampling,
             caps->max_buffer_size);
    }

    // Set API for PC sampling library
    level0_pc_hpcrun_api_set_fn = dlsym(level0_pc_lib_handle, "level0_pc_hpcrun_api_set");
    if (!level0_pc_hpcrun_api_set_fn) {
        EEMSG("Unable to find level0_pc_hpcrun_api_set in PC sampling library");
        init_result = LEVEL0_PC_ERROR_INIT_FAILED;
        dlclose(level0_pc_lib_handle);
        level0_pc_lib_handle = NULL;
        return;
    }

    // Pass the API table to the PC sampling library
    level0_pc_hpcrun_api_set_fn(&level0_pc_hpcrun_api);

    // Bind exported functions
    level0_pc_init_fn = dlsym(level0_pc_lib_handle, "level0_pc_init");
    level0_pc_shutdown_fn = dlsym(level0_pc_lib_handle, "level0_pc_shutdown");
    level0_pc_enabled_fn = dlsym(level0_pc_lib_handle, "level0_pc_enabled");
    level0_pc_profiler_create_fn = dlsym(level0_pc_lib_handle, "level0_pc_profiler_create");
    level0_pc_profiler_destroy_fn = dlsym(level0_pc_lib_handle, "level0_pc_profiler_destroy");
    level0_pc_update_correlation_id_fn = dlsym(level0_pc_lib_handle, "level0_pc_update_correlation_id");

    // Check that essential functions are present
    if (!level0_pc_init_fn || !level0_pc_shutdown_fn || !level0_pc_enabled_fn) {
        EEMSG("PC sampling library missing essential functions");
        init_result = LEVEL0_PC_ERROR_INIT_FAILED;
        dlclose(level0_pc_lib_handle);
        level0_pc_lib_handle = NULL;
        return;
    }

    init_result = LEVEL0_PC_SUCCESS;
    TMSG(LEVEL0, "PC Sampling: Library loaded successfully");
}


//******************************************************************************
// public interface functions
//******************************************************************************

level0_pc_result_t
level0_pc_init(const struct hpcrun_foil_appdispatch_level0* dispatch, char* error_buffer, size_t error_buffer_size)
{
    pthread_once(&once_control, init);

    if (init_result != LEVEL0_PC_SUCCESS) {
        return init_result;
    }

    if (level0_pc_init_fn) {
        level0_pc_result_t result = level0_pc_init_fn(dispatch, error_buffer, error_buffer_size);
        if (result != LEVEL0_PC_SUCCESS && error_buffer && error_buffer_size > 0) {
            EEMSG("PC sampling initialization failed: %s", error_buffer);
        }
        return result;
    }

    return LEVEL0_PC_ERROR_INIT_FAILED;
}


level0_pc_result_t
level0_pc_shutdown(void)
{
    pthread_once(&once_control, init);

    if (init_result != LEVEL0_PC_SUCCESS) {
        return init_result;
    }

    level0_pc_result_t result = LEVEL0_PC_ERROR_NOT_SUPPORTED;
    if (level0_pc_shutdown_fn) {
        result = level0_pc_shutdown_fn();
    }

    // Join any threads that might remain registered after the library shutdown
    pthread_mutex_lock(&thread_registry_mutex);
    while (thread_registry) {
        thread_registry_entry* entry = thread_registry;
        thread_registry = entry->next;
        pthread_mutex_unlock(&thread_registry_mutex);

        pthread_join(entry->thread, NULL);
        (void)entry;  // hpcrun_malloc memory reclaimed when pool resets

        pthread_mutex_lock(&thread_registry_mutex);
    }
    pthread_mutex_unlock(&thread_registry_mutex);

    return result;
}


bool
level0_pc_enabled(void)
{
    pthread_once(&once_control, init);

    if (init_result == LEVEL0_PC_SUCCESS && level0_pc_enabled_fn) {
        return level0_pc_enabled_fn();
    }

    return false;
}


void*
level0_pc_profiler_create(const struct hpcrun_foil_appdispatch_level0* dispatch, level0_pc_result_t* result)
{
    pthread_once(&once_control, init);

    if (init_result != LEVEL0_PC_SUCCESS) {
        if (result) *result = init_result;
        return NULL;
    }

    if (level0_pc_profiler_create_fn) {
        return level0_pc_profiler_create_fn(dispatch, result);
    }

    if (result) *result = LEVEL0_PC_ERROR_INIT_FAILED;
    return NULL;
}


void
level0_pc_profiler_destroy(void* profiler)
{
    pthread_once(&once_control, init);

    if (init_result == LEVEL0_PC_SUCCESS && level0_pc_profiler_destroy_fn) {
        level0_pc_profiler_destroy_fn(profiler);
    }
}


void
level0_pc_update_correlation_id(uint64_t cid, gpu_activity_channel_t* channel, void* context)
{
    pthread_once(&once_control, init);

    if (init_result == LEVEL0_PC_SUCCESS && level0_pc_update_correlation_id_fn) {
        level0_pc_update_correlation_id_fn(cid, channel, context);
    }
}
