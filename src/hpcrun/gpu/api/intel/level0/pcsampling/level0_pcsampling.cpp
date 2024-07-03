//==============================================================
// Copyright (C) Intel Corporation
//
// SPDX-License-Identifier: MIT
// =============================================================

#include <filesystem>
#include <iostream>
#include <pthread.h>

#include "level0_pcsampling.h"
#include "tracer.h"
#include "ze_metrics.h"

static UniTracer* tracer = nullptr;
static ZeMetricProfiler* metric_profiler = nullptr;

static pthread_once_t level0_pcsampling_init_once = PTHREAD_ONCE_INIT;
static std::string level0_pcsampling_enabled_str = (std::getenv("ZET_ENABLE_METRICS") ? std::getenv("ZET_ENABLE_METRICS") : "");

static const std::string base_path = "/tmp/hpcrun_level0_pc";
static char pattern[256];
static char* data_dir_name = nullptr;

static bool 
is_level0_pcsampling_enabled
(
  void
) 
{
  return level0_pcsampling_enabled_str == std::string("1");
}

static void 
EnableProfiling
(
  char *dir
) 
{
  metric_profiler = ZeMetricProfiler::Create(dir);
}

static void 
DisableProfiling
(
  void
) 
{
  if (metric_profiler != nullptr) {
    delete metric_profiler;
  }
}

void 
level0_pcsampling_init
(
  void
) 
{
  if (is_level0_pcsampling_enabled()) {
    if (!std::filesystem::exists(base_path)) {
        std::filesystem::create_directories(base_path);
        std::filesystem::permissions(base_path, std::filesystem::perms::all, std::filesystem::perm_options::add);
    }

    std::snprintf(pattern, sizeof(pattern), "%s/tmpdir.XXXXXX", base_path.c_str());
    data_dir_name = mkdtemp(pattern);
    if (data_dir_name == nullptr) {
      std::cerr << "[ERROR] Failed to create data folder" << std::endl;
      exit(-1);
    }
  }
}

static void 
level0_pcsampling_enable_helper
(
  void
) 
{
  EnableProfiling(data_dir_name);
  tracer = UniTracer::Create(data_dir_name); // kernel collector
}

void 
level0_pcsampling_enable
(
  void
) 
{
  if (is_level0_pcsampling_enabled()) {
    pthread_once(&level0_pcsampling_init_once, level0_pcsampling_enable_helper);
  }  
}

void 
level0_pcsampling_fini
(
  void
) 
{
  if (is_level0_pcsampling_enabled()) {
    delete tracer;
    DisableProfiling();
    for (const auto& e: std::filesystem::directory_iterator(std::filesystem::path(data_dir_name))) {
      std::filesystem::remove_all(e.path());
    }
    if (remove(data_dir_name)) {
      std::cerr << "[WARNING] " << data_dir_name << " is not removed. Please manually remove it." << std::endl;
    }
  }
}
