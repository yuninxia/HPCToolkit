//==============================================================
// Copyright (C) Intel Corporation
//
// SPDX-License-Identifier: MIT
// =============================================================

#include <array>
#include <csignal>
#include <filesystem>
#include <iostream>
#include <pthread.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/resource.h>

#include "level0_pcsampling.h"
#include "tracer.h"
#include "utils.h"
#include "ze_metrics.h"

static UniTracer* tracer = nullptr;
static ZeMetricProfiler* metric_profiler = nullptr;

static pthread_once_t level0_pcsampling_init_once = PTHREAD_ONCE_INIT;
static std::string level0_pcsampling_enabled = utils::GetEnv("ZET_ENABLE_METRICS");
static char pattern[] = "/tmp/intelpc/tmpdir.XXXXXX";
static char *data_dir = nullptr;
static std::string logfile;

static bool is_level0_pcsampling_enabled() {
    return level0_pcsampling_enabled == "1";
}

static void EnableProfiling(char *dir, std::string& logfile) {
  metric_profiler = ZeMetricProfiler::Create(dir, logfile);
}

static void DisableProfiling() {
  if (metric_profiler != nullptr) {
    delete metric_profiler;
  }
}

void level0_pcsampling_init() {
  if (is_level0_pcsampling_enabled()) {
    std::filesystem::path logDir = "/tmp/intelpc/";
    if (!std::filesystem::exists(logDir)) {
        std::filesystem::create_directories(logDir);
    }

    data_dir = mkdtemp(pattern);
    if (data_dir == nullptr) {
      std::cerr << "[ERROR] Failed to create data folder" << std::endl;
      exit(-1);
    }
    utils::SetEnv("Intel_PCSampling_DataDir", data_dir);

    logfile = logDir.string();
  }
}

static void level0_pcsampling_enable_helper() {
  EnableProfiling(data_dir, logfile);
  tracer = UniTracer::Create(); // kernel collector
}

void level0_pcsampling_enable() {
  if (is_level0_pcsampling_enabled()) {
    pthread_once(&level0_pcsampling_init_once, level0_pcsampling_enable_helper);
  }  
}

void level0_pcsampling_fini() {
  if (is_level0_pcsampling_enabled()) {
    delete tracer;
    DisableProfiling();
    for (const auto& e: std::filesystem::directory_iterator(std::filesystem::path(data_dir))) {
      std::filesystem::remove_all(e.path());
    }
    if (remove(data_dir)) {
      std::cerr << "[WARNING] " << data_dir << " is not removed. Please manually remove it." << std::endl;
    }
  }
}
