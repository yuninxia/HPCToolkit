// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: BSD-3-Clause

// -*-Mode: C++;-*-

//*****************************************************************************
// system includes
//*****************************************************************************

#include <cstdlib>
#include <cstring>


//*****************************************************************************
// local includes
//*****************************************************************************

#include "../pcsampling-hpcrun-api.h"
#include "pcsampling-api-receiver.hpp"
#include "level0-correlation-id.hpp"
#include "level0-pcsampling.hpp"


//*****************************************************************************
// local data
//*****************************************************************************

static bool level0_pc_initialized = false;


//*****************************************************************************
// exported interface (called from libhpcrun via shim)
//*****************************************************************************

extern "C" {

// NOTE: level0_pc_hpcrun_api_set, level0_pc_get_api_version, and
// level0_pc_get_capabilities are implemented in pcsampling-api-receiver.cpp.

__attribute__((visibility("default")))
level0_pc_result_t
level0_pc_init(
  const struct hpcrun_foil_appdispatch_level0* dispatch,
  char* error_buffer,
  size_t error_buffer_size
)
{
  if (!pcsampling::isInitialized()) {
    if (error_buffer && error_buffer_size > 0) {
      std::strncpy(error_buffer, "PC sampling API not initialized", error_buffer_size - 1);
      error_buffer[error_buffer_size - 1] = '\0';
    }
    return LEVEL0_PC_ERROR_INIT_FAILED;
  }

  if (level0_pc_initialized) {
    if (error_buffer && error_buffer_size > 0) {
      std::strncpy(error_buffer, "PC sampling already initialized", error_buffer_size - 1);
      error_buffer[error_buffer_size - 1] = '\0';
    }
    return LEVEL0_PC_SUCCESS;
  }

  level0PCSamplingInit(dispatch);

  if (!level0PCSamplingIsReady()) {
    if (error_buffer && error_buffer_size > 0) {
      std::strncpy(error_buffer, "Level Zero PC sampling initialization failed", error_buffer_size - 1);
      error_buffer[error_buffer_size - 1] = '\0';
    }
    return LEVEL0_PC_ERROR_INIT_FAILED;
  }

  level0_pc_initialized = true;
  return LEVEL0_PC_SUCCESS;
}


__attribute__((visibility("default")))
level0_pc_result_t
level0_pc_shutdown(void)
{
  if (!level0_pc_initialized) {
    return LEVEL0_PC_SUCCESS;
  }

  level0PCSamplingFini();
  level0_pc_initialized = false;
  return LEVEL0_PC_SUCCESS;
}


__attribute__((visibility("default")))
bool
level0_pc_enabled(void)
{
  return level0PCSamplingIsReady();
}


__attribute__((visibility("default")))
void*
level0_pc_profiler_create(
  const struct hpcrun_foil_appdispatch_level0* dispatch,
  level0_pc_result_t* result
)
{
  (void)dispatch;
  if (!level0_pc_initialized) {
    if (result) *result = LEVEL0_PC_ERROR_INIT_FAILED;
    return nullptr;
  }

  if (result) *result = LEVEL0_PC_SUCCESS;
  return reinterpret_cast<void*>(0x1);
}


__attribute__((visibility("default")))
void
level0_pc_profiler_destroy(void* profiler)
{
  (void)profiler;
}


__attribute__((visibility("default")))
void
level0_pc_update_correlation_id(
  uint64_t cid,
  gpu_activity_channel_t* channel,
  void* context
)
{
  level0UpdateCorrelationId(cid, channel, context);
}

} // extern "C"
