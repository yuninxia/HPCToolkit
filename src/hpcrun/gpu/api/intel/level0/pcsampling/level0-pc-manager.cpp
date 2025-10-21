// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: BSD-3-Clause

// -*-Mode: C++;-*-

//*****************************************************************************
// system includes
//*****************************************************************************

#include <pthread.h>


//*****************************************************************************
// local includes
//*****************************************************************************

#include "level0-pc-manager.hpp"
#include "level0-tracing-callbacks.hpp"
#include "level0-metric-profiler.hpp"
#include "level0-kernel-properties-cache.hpp"

extern "C" {
#include "../../../../../messages/messages.h"
}


//*****************************************************************************
// local variables
//*****************************************************************************

static ZeCollector* ze_collector = nullptr;
static ZeMetricProfiler* metric_profiler = nullptr;

static pthread_once_t level0_pc_init_once = PTHREAD_ONCE_INIT;
static std::string level0_pc_enabled_str = (std::getenv("ZET_ENABLE_METRICS") ? std::getenv("ZET_ENABLE_METRICS") : "");

// Thread-safe dispatch pointer for initialization
static const struct hpcrun_foil_appdispatch_level0* saved_dispatch = nullptr;
static bool level0_pc_init_succeeded = false;


//******************************************************************************
// private operations
//******************************************************************************

static bool
isPcSamplingEnabled
(
  void
)
{
  return level0_pc_enabled_str == "1";
}

static void
enableProfiling
(
  const struct hpcrun_foil_appdispatch_level0* dispatch
)
{
  metric_profiler = ZeMetricProfiler::Create(dispatch);
}

static void
disableProfiling
(
  void
)
{
  if (metric_profiler != nullptr) {
    ZeMetricProfiler::Destroy(metric_profiler);
    metric_profiler = nullptr;
  }
}

static void
pcSamplingEnableHelper
(
  const struct hpcrun_foil_appdispatch_level0* dispatch
)
{
  enableProfiling(dispatch);

  // Check if profiler was created successfully
  if (metric_profiler == nullptr) {
    EEMSG("Level0: Failed to create ZeMetricProfiler instance");
    level0_pc_init_succeeded = false;
    return;
  }

  ze_collector = ZeCollector::Create(dispatch);
  if (ze_collector == nullptr) {
    EEMSG("Level0: Failed to create ZeCollector instance");
    // Disable profiling since we couldn't initialize collector
    disableProfiling();
    level0_pc_init_succeeded = false;
  } else {
    level0_pc_init_succeeded = true;
  }
}


//******************************************************************************
// interface operations
//******************************************************************************

// PC Sampling Tile Attribution in Intel Level Zero:
//
// The behavior depends on the ZE_FLAT_DEVICE_HIERARCHY environment variable:
//
// 1. Explicit Scaling (ZE_FLAT_DEVICE_HIERARCHY=flat):
//    - Each tile appears as an independent device
//    - Each device has its own metric streamer
//    - callZetMetricGroupCalculateMultipleMetricValuesExp returns num_samples=1
//    - Simple 1:1 mapping between device and tile
//
// 2. Implicit Scaling (ZE_FLAT_DEVICE_HIERARCHY=composite or unset):
//    - Root devices manage multiple tiles (typically 2 per GPU)
//    - One metric streamer per root device collects data from all tiles
//    - callZetMetricGroupCalculateMultipleMetricValuesExp returns num_samples=<tile_count>
//    - The samples array index corresponds to tile index within the GPU:
//      * samples[0] = data from tile 0 of the GPU
//      * samples[1] = data from tile 1 of the GPU
//    - Actual tile ID = gpu_id * tiles_per_gpu + sample_index
//
// Tile attribution for PC sampling:
//    - Implicit scaling: samples array index maps to tile (samples[0]=tile0, samples[1]=tile1)
//    - Global tile ID = gpu_id * tiles_per_gpu + sample_index
//    - Tile info is preserved by encoding tile ID in IP address bits 48-63
//    - This allows accurate per-tile PC sample attribution in both scaling modes
//
void
level0PCSamplingInit
(
  const struct hpcrun_foil_appdispatch_level0* dispatch
)
{
  if (isPcSamplingEnabled()) {
    // Save the dispatch pointer in a static variable for use in the lambda
    saved_dispatch = dispatch;
    pthread_once(&level0_pc_init_once, []() { pcSamplingEnableHelper(saved_dispatch); });
  } else {
    TMSG(LEVEL0, "PC sampling is not enabled in the current configuration");
  }
}

void
level0PCSamplingFini
(
  void
)
{
  // Only cleanup if PC sampling was enabled AND successfully initialized
  if (isPcSamplingEnabled() && level0_pc_init_succeeded) {
    // Export cache statistics if in debug mode
    if (std::getenv("HPCTOOLKIT_LEVEL0_DEBUG_CACHE")) {
      auto stats = KernelPropertiesCache::getInstance().getStats();
      TMSG(LEVEL0, "PC Sampling Cache Statistics:");
      TMSG(LEVEL0, "  Total kernel entries: %zu", stats.total_entries);
      TMSG(LEVEL0, "  Devices tracked: %zu", stats.devices);
      TMSG(LEVEL0, "  Average read time: %lld us", stats.avg_read_time.count());
      TMSG(LEVEL0, "  Average write time: %lld us", stats.avg_write_time.count());
      TMSG(LEVEL0, "  Memory usage: %zu KB", stats.memory_bytes / 1024);
    }

    // Clean up collector resources
    if (ze_collector != nullptr) {
      ZeCollector::Destroy(ze_collector);
      ze_collector = nullptr;
    }

    // Clean up profiler resources
    disableProfiling();

    // Reset initialization state
    level0_pc_init_succeeded = false;
    saved_dispatch = nullptr;

    // Reset initialization flag to allow re-initialization if needed
    pthread_once_t once_init = PTHREAD_ONCE_INIT;
    level0_pc_init_once = once_init;
  }
}

bool
level0PCSamplingIsReady
(
  void
)
{
  return level0_pc_init_succeeded && isPcSamplingEnabled();
}
