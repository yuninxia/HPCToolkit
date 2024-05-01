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

#define LIB_UNITRACE_TOOL_NAME	"libunitrace_tool.so"

static UniTracer* tracer = nullptr;
static ZeMetricProfiler* metric_profiler = nullptr;

static pthread_once_t levelzero_pcsampling_enabled = PTHREAD_ONCE_INIT;
static char pattern[] = "/tmp/intelpc/tmpdir.XXXXXX";
static char *data_dir = nullptr;
std::string logfile;

void EnableProfiling(char *dir, std::string& logfile) {
#if 0
  if (zeInit(ZE_INIT_FLAG_GPU_ONLY) != ZE_RESULT_SUCCESS) {
    std::cerr << "[ERROR] Failed to initialize Level Zero runtime" << std::endl;
    std::cerr << "Please make sure /proc/sys/dev/i915/perf_stream_paranoid is set to 0." << std::endl;
    exit(-1);
  }
#endif
  metric_profiler = ZeMetricProfiler::Create(dir, logfile);
}

void DisableProfiling() {
  if (metric_profiler != nullptr) {
    delete metric_profiler;
  }
}

void levelzero_pcsampling_init() {

  utils::SetEnv("UNITRACE_MetricGroup", "EuStallSampling");
  utils::SetEnv("UNITRACE_KernelMetrics", "1");
  utils::SetEnv("UNITRACE_SamplingInterval", "50");

  utils::SetEnv("UNITRACE_LogToFile", "1");
  std::filesystem::path logDir = "/tmp/intelpc/";
  if (!std::filesystem::exists(logDir)) {
      std::filesystem::create_directories(logDir);
  }
  utils::SetEnv("UNITRACE_LogFilename", "/tmp/intelpc/");

  data_dir = mkdtemp(pattern);
  if (data_dir == nullptr) {
    std::cerr << "[ERROR] Failed to create data folder" << std::endl;
    exit(-1);
  }
  utils::SetEnv("UNITRACE_DataDir", data_dir);

  if (utils::GetEnv("UNITRACE_LogToFile") == "1") {
    logfile = utils::GetEnv("UNITRACE_LogFilename");
  }
}

void levelzero_pcsampling_enable_helper() {
  EnableProfiling(data_dir, logfile);
  tracer = UniTracer::Create(); // kernel collector
}

void levelzero_pcsampling_enable() {
  pthread_once(&levelzero_pcsampling_enabled, levelzero_pcsampling_enable_helper);
}

void levelzero_pcsampling_fini() {
  delete tracer;
  DisableProfiling();
  for (const auto& e: std::filesystem::directory_iterator(std::filesystem::path(data_dir))) {
    std::filesystem::remove_all(e.path());
  }
  if (remove(data_dir)) {
    std::cerr << "[WARNING] " << data_dir << " is not removed. Please manually remove it." << std::endl;
  }
}

