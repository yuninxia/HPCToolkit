// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: BSD-3-Clause

// -*-Mode: C++;-*-

//*****************************************************************************
// system includes
//*****************************************************************************

#include <iostream>
#include <pthread.h>


//*****************************************************************************
// local includes
//*****************************************************************************

#include "level0-pcsampling.hpp"
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

static pthread_once_t level0_pcsampling_init_once = PTHREAD_ONCE_INIT;
static std::string level0_pcsampling_enabled_str = (std::getenv("ZET_ENABLE_METRICS") ? std::getenv("ZET_ENABLE_METRICS") : "");

// Thread-safe dispatch pointer for initialization
static const struct hpcrun_foil_appdispatch_level0* saved_dispatch = nullptr;
static bool pcsampling_init_succeeded = false;


//******************************************************************************
// private operations
//******************************************************************************

static bool
isPcSamplingEnabled
(
  void
)
{
  return level0_pcsampling_enabled_str == "1";
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
    delete metric_profiler;
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
  ze_collector = ZeCollector::Create(dispatch);
  if (ze_collector == nullptr) {
    EEMSG("Level0: Failed to create ZeCollector instance");
    // Disable profiling since we couldn't initialize collector
    disableProfiling();
    pcsampling_init_succeeded = false;
  } else {
    pcsampling_init_succeeded = true;
  }
}


//******************************************************************************
// interface operations
//******************************************************************************

// device 1 - process 1 - thread 1
// device 2 - process 2 - thread 2
// FIXME(Yuning): How to collect data when using implicit scaling? multiple tiles under the same rank?
//                To check whether it is necessary to support this right now.
// TODO(Yuning): To put a runtime check for one tile per process.
// TODO(Yuning): To check the env var ZE_FLAT_DEVICE_HIERARCHY and implicit scaling. (quit)
// TODO(Yuning): The whole process maybe should be like: init, start, pause, resume, stop.
void
level0PCSamplingInit
(
  const struct hpcrun_foil_appdispatch_level0* dispatch
)
{
  if (isPcSamplingEnabled()) {
    // Save the dispatch pointer in a static variable for use in the lambda
    saved_dispatch = dispatch;
    pthread_once(&level0_pcsampling_init_once, []() { pcSamplingEnableHelper(saved_dispatch); });
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
  if (isPcSamplingEnabled() && pcsampling_init_succeeded) {
    // Export cache statistics if in debug mode
    if (std::getenv("HPCTOOLKIT_LEVEL0_DEBUG_CACHE")) {
      auto stats = KernelPropertiesCache::getInstance().getStats();
      // Debug output disabled for now to avoid linking issues
      // TODO: Re-enable once proper logging is implemented
      (void)stats;  // Suppress unused variable warning
    }

    // First stop the collector to ensure no new events are generated
    if (ze_collector != nullptr) {
      // Collector's destructor should handle proper shutdown
      delete ze_collector;
      ze_collector = nullptr;
    }

    // Then clean up profiler resources
    // This will wait for profiling threads to complete
    disableProfiling();

    // Reset initialization flag to allow re-initialization if needed
    pthread_once_t once_init = PTHREAD_ONCE_INIT;
    level0_pcsampling_init_once = once_init;
  }
}
