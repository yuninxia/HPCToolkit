// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: BSD-3-Clause

// -*-Mode: C++;-*-

//*****************************************************************************
// system includes
//*****************************************************************************

#include <cstring>
#include <string>
#include <cstdlib>


//*****************************************************************************
// local includes
//*****************************************************************************

#include "../pcsampling-hpcrun-api.h"
#include "pcsampling-api-receiver.hpp"
#include "level0-collector.hpp"
#include "level0-pcsampling.hpp"
#include "level0-correlation-id.hpp"


//*****************************************************************************
// global data
//*****************************************************************************

// Note: The collector and profiler are managed internally by level0PCSamplingInit/Fini
// They are stored in level0-pcsampling.cpp as ze_collector and metric_profiler
static bool pcsampling_initialized = false;


//*****************************************************************************
// exported functions (called from libhpcrun through shim)
//*****************************************************************************

extern "C" {

// This function has already been implemented in pcsampling-api-receiver.cpp
// uint32_t pcsampling_get_api_version(void);
// const pcsampling_capabilities_t* pcsampling_get_capabilities(void);
// void pcsampling_hpcrun_api_set(pcsampling_hpcrun_api_t* api);

__attribute__((visibility("default")))
pcsampling_result_t
pcsampling_init(
  const struct hpcrun_foil_appdispatch_level0* dispatch,
  char* error_buffer,
  size_t error_buffer_size
)
{
  if (!pcsampling::isInitialized()) {
    if (error_buffer && error_buffer_size > 0) {
      strncpy(error_buffer, "PC sampling API not initialized", error_buffer_size - 1);
      error_buffer[error_buffer_size - 1] = '\0';
    }
    return PCSAMPLING_ERROR_INIT_FAILED;
  }

  if (pcsampling_initialized) {
    if (error_buffer && error_buffer_size > 0) {
      strncpy(error_buffer, "PC sampling already initialized", error_buffer_size - 1);
      error_buffer[error_buffer_size - 1] = '\0';
    }
    return PCSAMPLING_SUCCESS;
  }

  try {
    // Initialize PC sampling - this creates the collector and metric profiler internally
    level0PCSamplingInit(dispatch);

    // Note: level0PCSamplingInit already creates:
    // - ZeCollector (stored in ze_collector)
    // - ZeMetricProfiler (stored in metric_profiler)
    // So we don't need to create them again here.

    pcsampling_initialized = true;
    return PCSAMPLING_SUCCESS;

  } catch (const std::exception& e) {
    if (error_buffer && error_buffer_size > 0) {
      strncpy(error_buffer, e.what(), error_buffer_size - 1);
      error_buffer[error_buffer_size - 1] = '\0';
    }
    return PCSAMPLING_ERROR_INIT_FAILED;
  } catch (...) {
    if (error_buffer && error_buffer_size > 0) {
      strncpy(error_buffer, "Unknown error during PC sampling initialization", error_buffer_size - 1);
      error_buffer[error_buffer_size - 1] = '\0';
    }
    return PCSAMPLING_ERROR_INIT_FAILED;
  }
}


__attribute__((visibility("default")))
pcsampling_result_t
pcsampling_shutdown(void)
{
  if (!pcsampling_initialized) {
    return PCSAMPLING_SUCCESS;
  }

  try {
    // Call the Level Zero PC sampling cleanup
    // This will clean up the collector and profiler that were created in level0PCSamplingInit
    level0PCSamplingFini();

    pcsampling_initialized = false;
    return PCSAMPLING_SUCCESS;

  } catch (...) {
    // Best effort cleanup
    pcsampling::error("Error during PC sampling shutdown");
    return PCSAMPLING_ERROR_INIT_FAILED;
  }
}


// Check if PC sampling is enabled via environment variables
static bool
level0_pcsampling_enabled_local(void)
{
  // PC sampling is enabled when ZET_ENABLE_METRICS=1 is set
  // This is set by hpcrun when using -e gpu=level0,pc
  const char* zet_metrics = getenv("ZET_ENABLE_METRICS");
  return (zet_metrics && strcmp(zet_metrics, "1") == 0);
}

__attribute__((visibility("default")))
bool
pcsampling_enabled(void)
{
  // Only check environment variables, not initialization state
  // This function determines if PC sampling should be enabled
  return level0_pcsampling_enabled_local();
}


__attribute__((visibility("default")))
void*
pcsampling_profiler_create(
  const struct hpcrun_foil_appdispatch_level0* dispatch,
  pcsampling_result_t* result
)
{
  if (!pcsampling_initialized) {
    if (result) *result = PCSAMPLING_ERROR_INIT_FAILED;
    return nullptr;
  }

  // The profiler is already created and managed by level0PCSamplingInit
  // We return a dummy non-null pointer to indicate success
  // The actual profiler is managed internally by level0-pcsampling.cpp
  if (result) *result = PCSAMPLING_SUCCESS;
  return (void*)0x1;  // Return a non-null dummy pointer
}


__attribute__((visibility("default")))
void
pcsampling_profiler_destroy(void* profiler)
{
  // The profiler is managed by level0PCSamplingInit/Fini
  // Nothing to do here
  (void)profiler;  // Suppress unused parameter warning
}


__attribute__((visibility("default")))
void
pcsampling_update_correlation_id(uint64_t cid, gpu_activity_channel_t* channel, void* context)
{
  // Forward to the existing implementation
  level0UpdateCorrelationId(cid, channel, context);
}

} // extern "C"