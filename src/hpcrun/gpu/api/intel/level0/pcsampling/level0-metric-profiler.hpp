// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: Apache-2.0
//
// SPDX-License-Identifier: MIT
// Copyright (C) 2024 Intel Corporation
// This file was inspired by and uses some code fragments from Intel's
// MIT-licensed pti-gpu (https://github.com/intel/pti-gpu)

// -*-Mode: C++;-*-

#ifndef LEVEL0_METRIC_PROFILER_H_
#define LEVEL0_METRIC_PROFILER_H_

struct hpcrun_foil_appdispatch_level0;

//*****************************************************************************
// system includes
//*****************************************************************************

#include <deque>
#include <map>
#include <vector>


//*****************************************************************************
// level zero includes
//*****************************************************************************

#include <level_zero/ze_api.h>
#include <level_zero/zet_api.h>


//*****************************************************************************
// local includes
//*****************************************************************************

#include "../../../../../libmonitor/monitor.h"
#include "../level0-command-process.h"
#include "level0-activity-generate.hpp"
#include "level0-activity-send.hpp"
#include "level0-activity-translate.hpp"
#include "level0-assert.hpp"
#include "level0-cmdlist-device-map.hpp"
#include "level0-correlation-id.hpp"
#include "level0-device.hpp"
#include "level0-kernel-properties.hpp"
#include "level0-metric-list.hpp"
#include "level0-metric-streamer.hpp"
#include "level0-metric.hpp"


//*****************************************************************************
// local variables
//*****************************************************************************

constexpr static uint32_t max_metric_size = 512;


//*****************************************************************************
// macros
//*****************************************************************************

#define MAX_METRIC_BUFFER (max_metric_samples * max_metric_size * 2)


//*****************************************************************************
// class definition
//*****************************************************************************

class ZeMetricProfiler {
 public:
  static ZeMetricProfiler* Create(const struct hpcrun_foil_appdispatch_level0* dispatch);
  static void Destroy(ZeMetricProfiler* profiler);
  ~ZeMetricProfiler();
  ZeMetricProfiler(const ZeMetricProfiler& that) = delete;
  ZeMetricProfiler& operator=(const ZeMetricProfiler& that) = delete;

 static void* ThreadMain(void* arg);

 private:
  ZeMetricProfiler(const struct hpcrun_foil_appdispatch_level0* dispatch);
  void StartProfilingMetrics();
  void StopProfilingMetrics();

  static void MetricProfilingThread(ZeMetricProfiler* profiler, ZeDeviceDescriptor *desc, const struct hpcrun_foil_appdispatch_level0* dispatch);
  static void RunProfilingLoop(ZeDeviceDescriptor* desc, zet_metric_streamer_handle_t& streamer, std::vector<std::string>& metric_list, const struct hpcrun_foil_appdispatch_level0* dispatch);
  static void CollectAndProcessMetrics(ZeDeviceDescriptor* desc, zet_metric_streamer_handle_t& streamer, std::vector<uint8_t>& raw_metrics, std::vector<std::string>& metric_list, const struct hpcrun_foil_appdispatch_level0* dispatch);

 private: // Data
  const struct hpcrun_foil_appdispatch_level0* dispatch_;
  std::vector<ze_context_handle_t> metric_contexts_;
};


#endif // LEVEL0_METRIC_PROFILER_H_
