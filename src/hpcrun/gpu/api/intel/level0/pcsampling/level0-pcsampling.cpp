// SPDX-FileCopyrightText: 2002-2024 Rice University
// SPDX-FileCopyrightText: 2024 Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: BSD-3-Clause

// -*-Mode: C++;-*-

//*****************************************************************************
// system includes
//*****************************************************************************

#include <filesystem>
#include <iostream>
#include <pthread.h>


//*****************************************************************************
// local includes
//*****************************************************************************

#include "level0-pcsampling.hpp"
#include "level0-tracing-callbacks.hpp"
#include "level0-metric-profiler.hpp"


//*****************************************************************************
// local variables
//*****************************************************************************

static ZeCollector* ze_collector = nullptr;
ZeMetricProfiler* metric_profiler = nullptr;

static pthread_once_t level0_pcsampling_init_once = PTHREAD_ONCE_INIT;
static std::string level0_pcsampling_enabled_str = (std::getenv("ZET_ENABLE_METRICS") ? std::getenv("ZET_ENABLE_METRICS") : "");

// Buffer for generating a unique directory name
static char pattern[256];
// Pointer to the created data directory name
static char* data_dir_name = nullptr;


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
  char* dir,
  const struct hpcrun_foil_appdispatch_level0* dispatch
)
{
  metric_profiler = ZeMetricProfiler::Create(dir, dispatch);
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
  enableProfiling(data_dir_name, dispatch);
  ze_collector = ZeCollector::Create(data_dir_name, dispatch);
  if (ze_collector == nullptr) {
    std::cerr << "[ERROR] Failed to create ZeCollector instance." << std::endl;
    exit(-1);
  }
}


//******************************************************************************
// interface operations
//******************************************************************************

void
zeroPCSamplingInit
(
  void
)
{
  const std::string base_path = "/tmp/hpcrun_level0_pc";
  // Create the base directory if it does not exist
  if (!std::filesystem::exists(base_path)) {
    std::filesystem::create_directories(base_path);
    // Grant all permissions to the base directory
    std::filesystem::permissions(base_path, std::filesystem::perms::all, std::filesystem::perm_options::add);
  }

  // Generate a unique temporary directory name using mkdtemp
  std::snprintf(pattern, sizeof(pattern), "%s/tmpdir.XXXXXX", base_path.c_str());
  data_dir_name = mkdtemp(pattern);
  if (data_dir_name == nullptr) {
    std::cerr << "[ERROR] Failed to create data folder '" << base_path << "'" << std::endl;
    exit(-1);
  }
}

void
zeroPCSamplingEnable
(
  const struct hpcrun_foil_appdispatch_level0* dispatch
)
{
  if (isPcSamplingEnabled()) {
    // Save the dispatch pointer in a static variable for use in the lambda
    static const struct hpcrun_foil_appdispatch_level0* saved_dispatch = dispatch;
    pthread_once(&level0_pcsampling_init_once, []() { pcSamplingEnableHelper(saved_dispatch); });
  } else {
    std::cerr << "[WARNING] PC sampling is not enabled in the current configuration." << std::endl;
  }
}

void
zeroPCSamplingFini
(
  void
)
{
  // Set to true if you want to keep the data directory for debugging purposes
  static const bool keep_data_dir_for_debug = false;

  if (isPcSamplingEnabled()) {
    if (ze_collector != nullptr) {
      delete ze_collector;
      ze_collector = nullptr;
    }
    disableProfiling();

    if (!keep_data_dir_for_debug) {
      std::error_code ec;
      // Recursively remove the data directory
      std::filesystem::remove_all(data_dir_name, ec);
      if (ec) {
        std::cerr << "[WARNING] Failed to remove " << data_dir_name 
                  << ". Please manually remove it." << std::endl;
      }
    }
  }
}
