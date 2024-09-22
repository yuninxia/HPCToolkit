// SPDX-FileCopyrightText: 2002-2024 Rice University
// SPDX-FileCopyrightText: 2024 Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: BSD-3-Clause

// -*-Mode: C++;-*-

#ifndef LEVEL0_METRIC_PROFILER_H_
#define LEVEL0_METRIC_PROFILER_H_

//*****************************************************************************
// system includes
//*****************************************************************************

#include <deque>
#include <map>
#include <mutex>
#include <thread>
#include <vector>


//******************************************************************************
// level0 includes
//******************************************************************************

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
#include "level0-buffer.hpp"
#include "level0-device.hpp"
#include "level0-kernel-properties.hpp"
#include "level0-metric.hpp"

extern "C" {
  #include "../../../../activity/correlation/gpu-correlation-channel.h"
  #include "../../../../activity/gpu-activity-channel.h"
}


//*****************************************************************************
// local variables
//*****************************************************************************

constexpr static uint32_t max_metric_size = 512;


//*****************************************************************************
// global variables
//*****************************************************************************

extern uint32_t max_metric_samples;


//*****************************************************************************
// macros
//*****************************************************************************

#define MAX_METRIC_BUFFER (max_metric_samples * max_metric_size * 2)


//*****************************************************************************
// class definition
//*****************************************************************************

class ZeMetricProfiler {
 public:
  static ZeMetricProfiler* Create(char *dir);
  ~ZeMetricProfiler();
  ZeMetricProfiler(const ZeMetricProfiler& that) = delete;
  ZeMetricProfiler& operator=(const ZeMetricProfiler& that) = delete;

  void GetDeviceDescriptors(std::map<ze_device_handle_t, ZeDeviceDescriptor*>& out_descriptors);
  void InsertCommandListDeviceMapping(ze_command_list_handle_t cmdList, ze_device_handle_t device);
  ze_device_handle_t GetDeviceForCommandList(ze_command_list_handle_t cmdList);

 private:
  ZeMetricProfiler();
  void StartProfilingMetrics();
  void StopProfilingMetrics();

  static void MetricProfilingThread(ZeMetricProfiler* profiler, ZeDeviceDescriptor *desc);
  static void RunProfilingLoop(ZeDeviceDescriptor* desc, zet_metric_streamer_handle_t& streamer);
  static void CollectAndProcessMetrics(ZeDeviceDescriptor* desc, zet_metric_streamer_handle_t& streamer, std::vector<uint8_t>& raw_metrics);
  static void UpdateCorrelationID(uint64_t cid, gpu_activity_channel_t *channel, void *arg);

 private: // Data
  static std::string data_dir_name_;
  std::vector<ze_context_handle_t> metric_contexts_;
  std::map<ze_device_handle_t, ZeDeviceDescriptor *> device_descriptors_;
  std::mutex cmdlist_device_map_mutex_;
  std::map<ze_command_list_handle_t, ze_device_handle_t> cmdlist_device_map_;
};


#endif // LEVEL0_METRIC_PROFILER_H_