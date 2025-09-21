// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: BSD-3-Clause

#ifndef _PCSAMPLING_SHIM_H_
#define _PCSAMPLING_SHIM_H_

//******************************************************************************
// system includes
//******************************************************************************

#include <stdbool.h>


//******************************************************************************
// type definitions
//******************************************************************************

// Error codes for PC sampling (duplicated here to avoid including internal API)
typedef enum {
    PCSAMPLING_SUCCESS = 0,
    PCSAMPLING_ERROR_INIT_FAILED = -1,
    PCSAMPLING_ERROR_LIBRARY_LOAD = -2,
    PCSAMPLING_ERROR_LEVEL0_API = -3,
    PCSAMPLING_ERROR_RESOURCE_EXHAUSTED = -4,
    PCSAMPLING_ERROR_VERSION_MISMATCH = -5,
    PCSAMPLING_ERROR_NOT_SUPPORTED = -6
} pcsampling_result_t;


//******************************************************************************
// forward declarations
//******************************************************************************

struct hpcrun_foil_appdispatch_level0;
struct gpu_activity_channel_t;


//******************************************************************************
// interface functions
//******************************************************************************

#ifdef __cplusplus
extern "C" {
#endif

// Initialize PC sampling subsystem
pcsampling_result_t pcsampling_init(const struct hpcrun_foil_appdispatch_level0* dispatch);

// Shutdown PC sampling subsystem
pcsampling_result_t pcsampling_shutdown(void);

// Check if PC sampling is enabled
bool pcsampling_enabled(void);

// Create a profiler instance
void* pcsampling_profiler_create(const struct hpcrun_foil_appdispatch_level0* dispatch,
                                 pcsampling_result_t* result);

// Destroy a profiler instance
void pcsampling_profiler_destroy(void* profiler);

// Update correlation ID
void pcsampling_update_correlation_id(uint64_t cid,
                                      struct gpu_activity_channel_t* channel,
                                      void* context);

#ifdef __cplusplus
}
#endif

#endif // _PCSAMPLING_SHIM_H_