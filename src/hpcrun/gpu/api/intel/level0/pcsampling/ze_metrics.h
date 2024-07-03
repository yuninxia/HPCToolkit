//==============================================================
// Copyright (C) Intel Corporation
//
// SPDX-License-Identifier: MIT
// =============================================================

#ifndef PTI_TOOLS_UNITRACE_LEVEL_ZERO_METRICS_H_
#define PTI_TOOLS_UNITRACE_LEVEL_ZERO_METRICS_H_

//*****************************************************************************
// system includes
//*****************************************************************************

#include <atomic>
#include <deque>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <mutex>
#include <sstream>
#include <string.h>
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
#include "../../../../activity/gpu-activity.h"
#include "../../../../activity/gpu-activity-channel.h"
#include "../../../../activity/correlation/gpu-correlation-channel.h"
#include "../level0-id-map.h"
#include "../level0-command-process.h"
#include "pti_assert.h"
#include "ze_utils.h"

//*****************************************************************************
// macros
//*****************************************************************************

constexpr static uint32_t max_metric_size = 512;
static uint32_t max_metric_samples = 65536;

#define MAX_METRIC_BUFFER  (max_metric_samples * max_metric_size * 2)

//*****************************************************************************
// type definitions
//*****************************************************************************

enum ZeProfilerState {
  PROFILER_DISABLED = 0,
  PROFILER_ENABLED = 1
};

struct ZeDeviceDescriptor {
  ze_device_handle_t device_;
  ze_device_handle_t parent_device_;
  ze_driver_handle_t driver_;
  ze_context_handle_t context_;
  int32_t device_id_;
  int32_t parent_device_id_;
  int32_t subdevice_id_;
  int32_t num_sub_devices_;
  zet_metric_group_handle_t metric_group_;
  std::thread *profiling_thread_;
  std::atomic<ZeProfilerState> profiling_state_;
  bool stall_sampling_;
};

struct EuStalls {
  uint64_t active_;
  uint64_t control_;
  uint64_t pipe_;
  uint64_t send_;
  uint64_t dist_;
  uint64_t sbid_;
  uint64_t sync_;
  uint64_t insfetch_;
  uint64_t other_;
};

struct KernelProperties {
  std::string name;
  uint64_t base_address;
  std::string kernel_id;
  std::string module_id;
  size_t size;
  size_t sample_count;
};

//*****************************************************************************
// class definition
//*****************************************************************************

class ZeMetricProfiler {
 public:
  static ZeMetricProfiler* Create(char *dir);
  ~ZeMetricProfiler();
  ZeMetricProfiler(const ZeMetricProfiler& that) = delete;
  ZeMetricProfiler& operator=(const ZeMetricProfiler& that) = delete;

 private:
  ZeMetricProfiler(char *dir);
  void StartProfilingMetrics();
  void StopProfilingMetrics();

  // Device enumeration
  ZeDeviceDescriptor* CreateDeviceDescriptor(ze_device_handle_t device, int32_t did, ze_driver_handle_t driver, ze_context_handle_t context, bool stall_sampling, const std::string& metric_group, char* dir);
  uint32_t GetSubDeviceCount(ze_device_handle_t device);
  zet_metric_group_handle_t GetMetricGroup(ze_device_handle_t device, const std::string& metric_group_name);
  void HandleSubDevices(ZeDeviceDescriptor* parent_desc);
  void EnumerateDevices(char *dir);
  int GetDeviceId(ze_device_handle_t sub_device) const;
  int GetSubDeviceId(ze_device_handle_t sub_device) const;
  ze_device_handle_t GetParentDevice(ze_device_handle_t sub_device) const;
  
  // GPU activity translation
  static gpu_inst_stall_t level0_convert_stall_reason(const EuStalls& stall);
  static bool level0_convert_pcsampling(gpu_activity_t* activity, std::map<uint64_t, EuStalls>::iterator it, std::map<uint64_t, KernelProperties>::const_reverse_iterator rit, uint64_t correlation_id);
  static void level0_activity_translate(std::deque<gpu_activity_t*>& activities, std::map<uint64_t, EuStalls>::iterator it, std::map<uint64_t, KernelProperties>::const_reverse_iterator rit, uint64_t correlation_id);
  template <typename T>
  static T hex_string_to_uint(const std::string& hex_str) {
    std::stringstream ss;
    T num = 0;
    ss << std::hex << hex_str;
    ss >> num;
    return num;
  }
  
  // Metric information
  static std::string GetMetricUnits(const char* units);
  static uint32_t GetMetricId(const std::vector<std::string>& metric_list, const std::string& metric_name);
  static uint32_t GetMetricCount(zet_metric_group_handle_t group);
  static std::vector<std::string> GetMetricList(zet_metric_group_handle_t group);

  // Process raw metrics
  static std::map<uint64_t, EuStalls> CalculateEuStalls(const ZeDeviceDescriptor* desc, int raw_size, const std::vector<uint8_t>& raw_metrics);
  static std::map<uint64_t, KernelProperties> ReadKernelProperties(const ZeDeviceDescriptor* desc);
  static std::deque<gpu_activity_t*> GenerateActivities(const std::map<uint64_t, KernelProperties>& kprops, std::map<uint64_t, EuStalls>& eustalls);
  static void ProcessActivities(std::deque<gpu_activity_t*>& activities); 
  static void LogActivities(const std::deque<gpu_activity_t*>& activities, const std::map<uint64_t, KernelProperties>& kprops); 

  // Collect and process metrics
  static uint64_t CollectMetrics(zet_metric_streamer_handle_t streamer, std::vector<uint8_t>& storage);
  static void ProcessRawMetrics(ZeMetricProfiler* profiler, ZeDeviceDescriptor* desc, const std::vector<uint8_t>& raw_metrics, int raw_size); 
  
  // Loop profiling
  static void CollectAndProcessMetrics(ZeMetricProfiler* profiler, ZeDeviceDescriptor* desc, zet_metric_streamer_handle_t& streamer);
  static zet_metric_streamer_handle_t FlushStreamerBuffer(zet_metric_streamer_handle_t old_streamer, ZeDeviceDescriptor* desc, ze_event_handle_t event, ze_event_pool_handle_t event_pool);
  static void NotifyDataProcessingComplete();
  static void UpdateCorrelationID(uint64_t cid, gpu_activity_channel_t *channel, void *arg);

  static void RunProfilingLoop(ZeMetricProfiler* profiler, ZeDeviceDescriptor* desc, ze_event_handle_t event, ze_event_pool_handle_t event_pool, zet_metric_streamer_handle_t& streamer);
  static void MetricProfilingThread(ZeMetricProfiler* profiler, ZeDeviceDescriptor *desc);

 private: // Data
  std::vector<ze_context_handle_t> metric_contexts_;
  std::map<ze_device_handle_t, ZeDeviceDescriptor *> device_descriptors_;
  static std::string data_dir_name_;
  static uint64_t correlation_id_;
  static uint64_t last_correlation_id_;
};

std::string ZeMetricProfiler::data_dir_name_;
uint64_t ZeMetricProfiler::last_correlation_id_ = 0;
uint64_t ZeMetricProfiler::correlation_id_ = 0;

ZeMetricProfiler* 
ZeMetricProfiler::Create
(
  char *dir
) 
{
  ZeMetricProfiler* profiler = new ZeMetricProfiler(dir);
  profiler->StartProfilingMetrics();
  return profiler;
}

ZeMetricProfiler::ZeMetricProfiler
(
  char *dir
) 
{
  data_dir_name_ = std::string(dir);
  EnumerateDevices(dir);
}

ZeMetricProfiler::~ZeMetricProfiler
(
  void
) 
{
  StopProfilingMetrics();
}

void 
ZeMetricProfiler::StartProfilingMetrics
(
  void
) 
{
  for (auto it = device_descriptors_.begin(); it != device_descriptors_.end(); ++it) {
    if (it->second->parent_device_ != nullptr) {
      // Skip subdevices
      continue;
    }
    monitor_disable_new_threads();
    it->second->profiling_thread_ = new std::thread(MetricProfilingThread, this, it->second);
    monitor_enable_new_threads();
    while (it->second->profiling_state_.load(std::memory_order_acquire) != PROFILER_ENABLED) {
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
  }
}

void 
ZeMetricProfiler::StopProfilingMetrics
(
  void
) 
{
  for (auto it = device_descriptors_.begin(); it != device_descriptors_.end(); ++it) {
    if (it->second->parent_device_ != nullptr) {
      // Skip subdevices
      continue;
    }
    PTI_ASSERT(it->second->profiling_thread_ != nullptr);
    PTI_ASSERT(it->second->profiling_state_ == PROFILER_ENABLED);
    it->second->profiling_state_.store(PROFILER_DISABLED, std::memory_order_release);
    it->second->profiling_thread_->join();
    delete it->second->profiling_thread_;
    it->second->profiling_thread_ = nullptr;
  }
}

zet_metric_group_handle_t
ZeMetricProfiler::GetMetricGroup
(
  ze_device_handle_t device, 
  const std::string& metric_group_name
) 
{
  uint32_t num_groups = 0;
  ze_result_t status = zetMetricGroupGet(device, &num_groups, nullptr);
  PTI_ASSERT(status == ZE_RESULT_SUCCESS);

  if (num_groups == 0) {
    std::cerr << "[WARNING] No metric groups found" << std::endl;
    return nullptr;
  }

  zet_metric_group_handle_t group = nullptr;
  
  std::vector<zet_metric_group_handle_t> groups(num_groups, nullptr);
  status = zetMetricGroupGet(device, &num_groups, groups.data());
  PTI_ASSERT(status == ZE_RESULT_SUCCESS);

  for (uint32_t k = 0; k < num_groups; ++k) {
    zet_metric_group_properties_t group_props{};
    group_props.stype = ZET_STRUCTURE_TYPE_METRIC_GROUP_PROPERTIES;
    status = zetMetricGroupGetProperties(groups[k], &group_props);
    PTI_ASSERT(status == ZE_RESULT_SUCCESS);

    if ((strcmp(group_props.name, metric_group_name.c_str()) == 0) && (group_props.samplingType & ZET_METRIC_GROUP_SAMPLING_TYPE_FLAG_TIME_BASED)) {
      group = groups[k];
      break;
    }
  }

  if (group == nullptr) {
    std::cerr << "[ERROR] Invalid metric group " << metric_group_name << std::endl;
    exit(-1);
  }

  return group;
}


ZeDeviceDescriptor* 
ZeMetricProfiler::CreateDeviceDescriptor
(
  ze_device_handle_t device, 
  int32_t did, 
  ze_driver_handle_t driver, 
  ze_context_handle_t context, 
  bool stall_sampling, 
  const std::string& metric_group, 
  char* dir
) 
{
  ZeDeviceDescriptor *desc = new ZeDeviceDescriptor;

  desc->stall_sampling_ = stall_sampling;
  desc->device_ = device;
  desc->device_id_ = did;
  desc->parent_device_id_ = -1;    // no parent device
  desc->parent_device_ = nullptr;
  desc->subdevice_id_ = -1;        // not a subdevice
  desc->num_sub_devices_ = GetSubDeviceCount(device);
  desc->driver_ = driver;
  desc->context_ = context;
  desc->metric_group_ = GetMetricGroup(device, metric_group);
  desc->profiling_thread_ = nullptr;
  desc->profiling_state_.store(PROFILER_DISABLED, std::memory_order_release);

  return desc;
}

uint32_t 
ZeMetricProfiler::GetSubDeviceCount
(
  ze_device_handle_t device
) 
{
  uint32_t num_sub_devices = 0;
  ze_result_t status = zeDeviceGetSubDevices(device, &num_sub_devices, nullptr);
  PTI_ASSERT(status == ZE_RESULT_SUCCESS);
  return num_sub_devices;
}

void 
ZeMetricProfiler::HandleSubDevices
(
  ZeDeviceDescriptor* parent_desc
)
{
  uint32_t num_sub_devices = GetSubDeviceCount(parent_desc->device_);
  if (num_sub_devices == 0) return;

  std::vector<ze_device_handle_t> sub_devices(num_sub_devices);
  ze_result_t status = zeDeviceGetSubDevices(parent_desc->device_, &num_sub_devices, sub_devices.data());
  PTI_ASSERT(status == ZE_RESULT_SUCCESS);

  for (uint32_t j = 0; j < num_sub_devices; j++) {
    ZeDeviceDescriptor* sub_desc = new ZeDeviceDescriptor;
    sub_desc->stall_sampling_ = parent_desc->stall_sampling_;
    sub_desc->device_ = sub_devices[j];
    sub_desc->device_id_ = parent_desc->device_id_;
    sub_desc->parent_device_id_ = parent_desc->device_id_;
    sub_desc->parent_device_ = parent_desc->device_;
    sub_desc->subdevice_id_ = j;
    sub_desc->num_sub_devices_ = 0;
    sub_desc->driver_ = parent_desc->driver_;
    sub_desc->context_ = parent_desc->context_;
    sub_desc->metric_group_ = parent_desc->metric_group_;
    sub_desc->profiling_thread_ = nullptr;
    sub_desc->profiling_state_.store(PROFILER_DISABLED, std::memory_order_release);

    device_descriptors_.insert({sub_devices[j], sub_desc});
  }
}

void 
ZeMetricProfiler::EnumerateDevices
(
  char *dir
) 
{
  std::string metric_group = "EuStallSampling";
  bool stall_sampling = (metric_group == "EuStallSampling");

  uint32_t num_drivers = 0;
  ze_result_t status = zeDriverGet(&num_drivers, nullptr);
  PTI_ASSERT(status == ZE_RESULT_SUCCESS);

  if (num_drivers == 0) {
    return;
  }

  std::vector<ze_driver_handle_t> drivers(num_drivers);
  status = zeDriverGet(&num_drivers, drivers.data());
  PTI_ASSERT(status == ZE_RESULT_SUCCESS);

  int32_t did = 0;
  for (auto driver : drivers) {
    ze_context_handle_t context = nullptr;
    ze_context_desc_t cdesc = {ZE_STRUCTURE_TYPE_CONTEXT_DESC, nullptr, 0};
    status = zeContextCreate(driver, &cdesc, &context);
    PTI_ASSERT(status == ZE_RESULT_SUCCESS);
    metric_contexts_.push_back(context);

    uint32_t num_devices = 0;
    status = zeDeviceGet(driver, &num_devices, nullptr);
    PTI_ASSERT(status == ZE_RESULT_SUCCESS);
    if (num_devices == 0) continue;

    std::vector<ze_device_handle_t> devices(num_devices);
    status = zeDeviceGet(driver, &num_devices, devices.data());
    PTI_ASSERT(status == ZE_RESULT_SUCCESS);

    for (auto device : devices) {
      ZeDeviceDescriptor* desc = CreateDeviceDescriptor(device, did, driver, context, stall_sampling, metric_group, dir);
      if (desc == nullptr) {
        continue;
      }

      device_descriptors_.insert({device, desc});
      HandleSubDevices(desc);
      did++;
    }
  }
}

int 
ZeMetricProfiler::GetDeviceId
(
  ze_device_handle_t sub_device
) const 
{
  auto it = device_descriptors_.find(sub_device);
  if (it != device_descriptors_.end()) {
    return it->second->device_id_;
  }
  return -1;
}

int 
ZeMetricProfiler::GetSubDeviceId
(
  ze_device_handle_t sub_device
) const 
{
  auto it = device_descriptors_.find(sub_device);
  if (it != device_descriptors_.end()) {
    return it->second->subdevice_id_;
  }
  return -1;
}

ze_device_handle_t 
ZeMetricProfiler::GetParentDevice
(
  ze_device_handle_t sub_device
) const 
{
  auto it = device_descriptors_.find(sub_device);
  if (it != device_descriptors_.end()) {
    return it->second->parent_device_;
  }
  return nullptr;
}

gpu_inst_stall_t 
ZeMetricProfiler::level0_convert_stall_reason
(
  const EuStalls& stall
) 
{
  gpu_inst_stall_t stall_reason = GPU_INST_STALL_NONE;
  uint64_t max_value = 0;

  if (stall.control_ > max_value) {
    max_value = stall.control_;
    stall_reason = GPU_INST_STALL_OTHER; // TBD
  }
  if (stall.pipe_ > max_value) {
    max_value = stall.pipe_;
    stall_reason = GPU_INST_STALL_PIPE_BUSY;
  }
  if (stall.send_ > max_value) {
    max_value = stall.send_;
    stall_reason = GPU_INST_STALL_GMEM; // TBD
  }
  if (stall.dist_ > max_value) {
    max_value = stall.dist_;
    stall_reason = GPU_INST_STALL_PIPE_BUSY; // TBD
  }
  if (stall.sbid_ > max_value) {
    max_value = stall.sbid_;
    stall_reason = GPU_INST_STALL_IDEPEND; // TBD
  }
  if (stall.sync_ > max_value) {
    max_value = stall.sync_;
    stall_reason = GPU_INST_STALL_SYNC;
  }
  if (stall.insfetch_ > max_value) {
    max_value = stall.insfetch_;
    stall_reason = GPU_INST_STALL_IFETCH;
  }
  if (stall.other_ > max_value) {
    max_value = stall.other_;
    stall_reason = GPU_INST_STALL_OTHER;
  }

  return stall_reason;
}

bool 
ZeMetricProfiler::level0_convert_pcsampling
(
  gpu_activity_t* activity, 
  std::map<uint64_t, EuStalls>::iterator it,
  std::map<uint64_t, KernelProperties>::const_reverse_iterator rit,
  uint64_t correlation_id
) 
{
  activity->kind = GPU_ACTIVITY_PC_SAMPLING;

  KernelProperties kernel_props = rit->second;
  EuStalls stall = it->second;

  uint32_t module_id_uint32 = hex_string_to_uint<uint32_t>(kernel_props.module_id.substr(0, 8));
  zebin_id_map_entry_t *entry = zebin_id_map_lookup(module_id_uint32);
  if (entry != nullptr) {
    uint32_t hpctoolkit_module_id = zebin_id_map_entry_hpctoolkit_id_get(entry);
    activity->details.pc_sampling.pc.lm_id = (uint16_t)hpctoolkit_module_id;
  }

  activity->details.pc_sampling.pc.lm_ip = it->first + 0x800000000000; // real = it->first; base = rit->first; offset = real - base;
  activity->details.pc_sampling.correlation_id = correlation_id;
  activity->details.pc_sampling.samples = stall.active_ + stall.control_ + stall.pipe_ + stall.send_ + stall.dist_ + stall.sbid_ + stall.sync_ + stall.insfetch_ + stall.other_;
  activity->details.pc_sampling.latencySamples = activity->details.pc_sampling.samples - stall.active_;
  activity->details.pc_sampling.stallReason = level0_convert_stall_reason(stall);

  return true;
}

void 
ZeMetricProfiler::level0_activity_translate
(
  std::deque<gpu_activity_t*>& activities, 
  std::map<uint64_t, EuStalls>::iterator it,
  std::map<uint64_t, KernelProperties>::const_reverse_iterator rit,
  uint64_t correlation_id
) 
{
  gpu_activity_t* activity = new gpu_activity_t();
  gpu_activity_init(activity);

  if (level0_convert_pcsampling(activity, it, rit, correlation_id)) {
    activities.push_back(activity);
  } else {
    delete activity;
  }
}

std::map<uint64_t, KernelProperties> 
ZeMetricProfiler::ReadKernelProperties
(
  const ZeDeviceDescriptor* desc
) 
{
  std::map<uint64_t, KernelProperties> kprops;
  // enumerate all kernel property files
  for (const auto& e : std::filesystem::directory_iterator(std::filesystem::path(data_dir_name_))) {
    // kernel properties file path: <data_dir>/.kprops.<device_id>.<pid>.txt
    if (e.path().filename().string().find(".kprops." + std::to_string(desc->device_id_)) == 0) {
      std::ifstream kpf = std::ifstream(e.path());
      if (!kpf.is_open()) {
        continue;
      }

      while (!kpf.eof()) {
        KernelProperties props;
        std::string line;

        std::getline(kpf, props.name);
        if (kpf.eof()) {
          break;
        }

        std::getline(kpf, line);
        if (kpf.eof()) {
          break;
        }
        props.base_address = std::strtol(line.c_str(), nullptr, 0);

        std::getline(kpf, props.kernel_id);
        std::getline(kpf, props.module_id);

        line.clear();
        std::getline(kpf, line);
        props.size = std::strtol(line.c_str(), nullptr, 0);

        kprops.insert({props.base_address, std::move(props)});
      }
      kpf.close();
    }
  }
  return kprops;
}

std::deque<gpu_activity_t*>
ZeMetricProfiler::GenerateActivities
(
  const std::map<uint64_t, KernelProperties>& kprops, 
  std::map<uint64_t, EuStalls>& eustalls
) 
{
  std::deque<gpu_activity_t*> activities;
  std::unordered_map<std::string, uint64_t> kernel_cids;
  for (auto rit = kprops.crbegin(); rit != kprops.crend(); ++rit) {
    if (kernel_cids.find(rit->second.name) == kernel_cids.end()) {
      kernel_cids[rit->second.name] = correlation_id_;
    }
  }

  for (auto it = eustalls.begin(); it != eustalls.end(); ++it) {
    for (auto rit = kprops.crbegin(); rit != kprops.crend(); ++rit) {
      if ((rit->first <= it->first) && ((it->first - rit->first) < rit->second.size)) {
        uint64_t cid = kernel_cids[rit->second.name];
        level0_activity_translate(activities, it, rit, cid);
        break;
      }
    }
  }

  return activities;
}

void 
ZeMetricProfiler::ProcessActivities
(
  std::deque<gpu_activity_t*>& activities
) 
{
  for (auto activity : activities) {
    uint32_t thread_id = gpu_activity_channel_correlation_id_get_thread_id(activity->details.instruction.correlation_id);
    gpu_activity_channel_t *channel = gpu_activity_channel_lookup(thread_id);
    gpu_activity_channel_send(channel, activity);
  }
}

std::string 
ZeMetricProfiler::GetMetricUnits
(
  const char* units
) 
{
  PTI_ASSERT(units != nullptr);

  std::string result = units;
  if (result.find("null") != std::string::npos) {
    result = "";
  } else if (result.find("percent") != std::string::npos) {
    result = "%";
  }

  return result;
}

uint32_t 
ZeMetricProfiler::GetMetricId
(
  const std::vector<std::string>& metric_list, 
  const std::string& metric_name
) 
{
  PTI_ASSERT(!metric_list.empty());
  PTI_ASSERT(!metric_name.empty());

  for (size_t i = 0; i < metric_list.size(); ++i) {
    if (metric_list[i].find(metric_name) == 0) {
      return i;
    }
  }

  return metric_list.size();
}

uint32_t 
ZeMetricProfiler::GetMetricCount
(
  zet_metric_group_handle_t group
) 
{
  PTI_ASSERT(group != nullptr);

  zet_metric_group_properties_t group_props{};
  group_props.stype = ZET_STRUCTURE_TYPE_METRIC_GROUP_PROPERTIES;
  ze_result_t status = zetMetricGroupGetProperties(group, &group_props);
  PTI_ASSERT(status == ZE_RESULT_SUCCESS);

  return group_props.metricCount;
}

std::vector<std::string> 
ZeMetricProfiler::GetMetricList
(
  zet_metric_group_handle_t group
) 
{
  PTI_ASSERT(group != nullptr);

  uint32_t metric_count = GetMetricCount(group);
  PTI_ASSERT(metric_count > 0);

  std::vector<zet_metric_handle_t> metric_list(metric_count);
  ze_result_t status = zetMetricGet(group, &metric_count, metric_list.data());
  PTI_ASSERT(status == ZE_RESULT_SUCCESS);
  PTI_ASSERT(metric_count == metric_list.size());

  std::vector<std::string> name_list;
  for (auto metric : metric_list) {
    zet_metric_properties_t metric_props{
      ZET_STRUCTURE_TYPE_METRIC_PROPERTIES,
    };
    status = zetMetricGetProperties(metric, &metric_props);
    PTI_ASSERT(status == ZE_RESULT_SUCCESS);

    std::string units = GetMetricUnits(metric_props.resultUnits);
    std::string name = metric_props.name;
    if (!units.empty()) {
      name += "[" + units + "]";
    }
    name_list.push_back(name);
  }

  return name_list;
}

uint64_t 
ZeMetricProfiler::CollectMetrics
(
  zet_metric_streamer_handle_t streamer, 
  std::vector<uint8_t>& storage
)
{
  ze_result_t status = ZE_RESULT_SUCCESS;
  size_t data_size = 0;

  status = zetMetricStreamerReadData(streamer, UINT32_MAX, &data_size, nullptr);
  PTI_ASSERT(status == ZE_RESULT_SUCCESS);
  PTI_ASSERT(data_size > 0);

  if (data_size > storage.size()) {
    data_size = storage.size();
    std::cerr << "[WARNING] Metric samples dropped." << std::endl;
  }

  status = zetMetricStreamerReadData(streamer, UINT32_MAX, &data_size, storage.data());
  PTI_ASSERT(status == ZE_RESULT_SUCCESS);

  return data_size;
}

void 
ZeMetricProfiler::UpdateCorrelationID
(
  uint64_t cid, 
  gpu_activity_channel_t *channel, 
  void *arg
) 
{
  correlation_id_ = cid;
}

zet_metric_streamer_handle_t 
ZeMetricProfiler::FlushStreamerBuffer
(
  zet_metric_streamer_handle_t old_streamer, 
  ZeDeviceDescriptor* desc,
  ze_event_handle_t event,
  ze_event_pool_handle_t event_pool
) 
{
  ze_result_t status = ZE_RESULT_SUCCESS;

  // Close the old streamer
  status = zetMetricStreamerClose(old_streamer);
  PTI_ASSERT(status == ZE_RESULT_SUCCESS);

  // Open a new streamer
  zet_metric_streamer_handle_t new_streamer = nullptr;
  uint32_t interval = 50 * 1000; // ns
  zet_metric_streamer_desc_t streamer_desc = { ZET_STRUCTURE_TYPE_METRIC_STREAMER_DESC, nullptr, max_metric_samples, interval };
  status = zetMetricStreamerOpen(desc->context_, desc->device_, desc->metric_group_, &streamer_desc, event, &new_streamer);
  if (status != ZE_RESULT_SUCCESS) {
    std::cerr << "[ERROR] Failed to open metric streamer (" << status << "). The sampling interval might be too small." << std::endl;
    status = zeEventDestroy(event);
    PTI_ASSERT(status == ZE_RESULT_SUCCESS);
    status = zeEventPoolDestroy(event_pool);
    PTI_ASSERT(status == ZE_RESULT_SUCCESS);
    return nullptr;
  }

  if (streamer_desc.notifyEveryNReports > max_metric_samples) {
    max_metric_samples = streamer_desc.notifyEveryNReports;
  }

  return new_streamer;
}

void 
ZeMetricProfiler::LogActivities
(
  const std::deque<gpu_activity_t*>& activities,
  const std::map<uint64_t, KernelProperties>& kprops
)
{
  std::map<uint64_t, std::pair<std::string, uint64_t>> kernel_info;
  for (const auto& [base, prop] : kprops) {
    uint64_t adjusted_base = base + 0x800000000000;
    kernel_info[adjusted_base] = {prop.name, adjusted_base};
  }

  std::cout << std::endl;

  std::unordered_map<uint64_t, int> cid_count;

  for (const auto* activity : activities) {
    uint64_t instruction_pc_lm_ip = activity->details.instruction.pc.lm_ip;
    uint64_t cid = activity->details.pc_sampling.correlation_id;
    uint16_t lm_id = activity->details.pc_sampling.pc.lm_id;
    
    auto it = kernel_info.upper_bound(instruction_pc_lm_ip);
    if (it != kernel_info.begin()) {
      --it;
    }
    const auto& [kernel_name, kernel_base] = (it != kernel_info.end()) ? it->second : std::pair<std::string, uint64_t>("Unknown", 0);

    uint64_t offset = (kernel_base != 0) ? (instruction_pc_lm_ip - kernel_base) : 0;

    std::cerr << "PC Sample" << std::endl;
    std::cerr << "PC sampling: sample(pc=0x" << std::hex << activity->details.pc_sampling.pc.lm_ip
              << ", cid=" << cid
              << ", kernel_name=" << kernel_name
              << ")" << std::endl;
    std::cerr << "PC sampling: normalize 0x" << std::hex << instruction_pc_lm_ip
              << " --> [" << std::dec << lm_id << ", 0x" 
              << std::hex << offset << "]" << std::endl;

    cid_count[cid]++;
  }

  std::cout << std::dec << std::endl;
  std::cout << "Correlation ID Statistics:" << std::endl;
  for (const auto& [cid, count] : cid_count) {
    std::cout << "Correlation ID: " << cid << " Count: " << count << std::endl;
  }
}

std::map<uint64_t, EuStalls>
ZeMetricProfiler::CalculateEuStalls
(
  const ZeDeviceDescriptor* desc,
  int raw_size,
  const std::vector<uint8_t>& raw_metrics
)
{
  std::map<uint64_t, EuStalls> eustalls;

  std::vector<std::string> metric_list = GetMetricList(desc->metric_group_);
  PTI_ASSERT(!metric_list.empty());

  uint32_t ip_idx = GetMetricId(metric_list, "IP");
  if (ip_idx >= metric_list.size()) {
    // no IP metric 
    return eustalls;
  }

  uint32_t num_samples = 0;
  uint32_t num_metrics = 0;

  ze_result_t status = zetMetricGroupCalculateMultipleMetricValuesExp(
    desc->metric_group_, ZET_METRIC_GROUP_CALCULATION_TYPE_METRIC_VALUES,
    raw_size, raw_metrics.data(), &num_samples, &num_metrics,
    nullptr, nullptr);

  if ((status != ZE_RESULT_SUCCESS) || (num_samples == 0) || (num_metrics == 0)) {
    std::cerr << "[WARNING] Unable to calculate metrics" << std::endl;
    return eustalls;
  }

  std::vector<uint32_t> samples(num_samples);
  std::vector<zet_typed_value_t> metrics(num_metrics);
  status = zetMetricGroupCalculateMultipleMetricValuesExp(
    desc->metric_group_, ZET_METRIC_GROUP_CALCULATION_TYPE_METRIC_VALUES,
    raw_size, raw_metrics.data(), &num_samples, &num_metrics,
    samples.data(), metrics.data());

  if ((status != ZE_RESULT_SUCCESS) && (status != ZE_RESULT_WARNING_DROPPED_DATA)) {
    std::cerr << "[WARNING] Unable to calculate metrics" << std::endl;
    return eustalls;
  }

  const zet_typed_value_t *value = metrics.data();

  for (uint32_t i = 0; i < num_samples; ++i) {
    uint32_t size = samples[i];
    for (uint32_t j = 0; j < size; j += metric_list.size()) {
      uint64_t ip = (value[j + 0].value.ui64 << 3);
      if (ip == 0) {
        continue;
      }
      EuStalls stall;
      stall.active_ = value[j + 1].value.ui64;
      stall.control_ = value[j + 2].value.ui64;
      stall.pipe_ = value[j + 3].value.ui64;
      stall.send_ = value[j + 4].value.ui64;
      stall.dist_ = value[j + 5].value.ui64;
      stall.sbid_ = value[j + 6].value.ui64;
      stall.sync_ = value[j + 7].value.ui64;
      stall.insfetch_ = value[j + 8].value.ui64;
      stall.other_ = value[j + 9].value.ui64;
      auto [it, inserted] = eustalls.try_emplace(ip, stall);
      if (!inserted) {
        EuStalls& existing = it->second;
        existing.active_ += stall.active_;
        existing.control_ += stall.control_;
        existing.pipe_ += stall.pipe_;
        existing.send_ += stall.send_;
        existing.dist_ += stall.dist_;
        existing.sbid_ += stall.sbid_;
        existing.sync_ += stall.sync_;
        existing.insfetch_ += stall.insfetch_;
        existing.other_ += stall.other_;
      }
    }
    value += samples[i];
  }

  return eustalls;
}

void 
ZeMetricProfiler::ProcessRawMetrics
(
  ZeMetricProfiler* profiler,
  ZeDeviceDescriptor* desc, 
  const std::vector<uint8_t>& raw_metrics, 
  int raw_size
) 
{

  std::map<uint64_t, EuStalls> eustalls = CalculateEuStalls(desc, raw_size, raw_metrics);
  if (eustalls.size() == 0) {
    return;
  }

  std::map<uint64_t, KernelProperties> kprops = ReadKernelProperties(desc);
  if (kprops.size() == 0) {
    return;
  }

  std::deque<gpu_activity_t*> activities = GenerateActivities(kprops, eustalls);
  ProcessActivities(activities);
#if 1
  LogActivities(activities, kprops);
#endif
  for (auto activity : activities) {
    delete activity;
  }
}

void 
ZeMetricProfiler::CollectAndProcessMetrics
(
  ZeMetricProfiler* profiler, 
  ZeDeviceDescriptor* desc, 
  zet_metric_streamer_handle_t& streamer
)
{
  std::vector<uint8_t> raw_metrics(MAX_METRIC_BUFFER + 512);

  uint64_t size = CollectMetrics(streamer, raw_metrics);

  if (size > 0) {
    ProcessRawMetrics(profiler, desc, raw_metrics, size);
  }
}

void 
ZeMetricProfiler::NotifyDataProcessingComplete
(
  void
)
{
  pthread_mutex_lock(&gpu_activity_mtx);
  data_processed = true;
  pthread_cond_signal(&cv);
  pthread_mutex_unlock(&gpu_activity_mtx);
}

void 
ZeMetricProfiler::RunProfilingLoop
(
  ZeMetricProfiler* profiler, 
  ZeDeviceDescriptor* desc, 
  ze_event_handle_t event, 
  ze_event_pool_handle_t event_pool,
  zet_metric_streamer_handle_t& streamer
) 
{
  std::vector<uint8_t> raw_metrics(MAX_METRIC_BUFFER + 512);
  desc->profiling_state_.store(PROFILER_ENABLED, std::memory_order_release);
  
  while (desc->profiling_state_.load(std::memory_order_acquire) != PROFILER_DISABLED) {
    pthread_mutex_lock(&kernel_mutex);
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec += 1;
    
    // Wait for the kernel to start running
    while (!kernel_running) {
      int rc = pthread_cond_timedwait(&kernel_cond, &kernel_mutex, &ts);
      if (rc == ETIMEDOUT) {
        // Check if profiling should be terminated
        if (desc->profiling_state_.load(std::memory_order_acquire) == PROFILER_DISABLED) {
          pthread_mutex_unlock(&kernel_mutex);
          return;
        }
        // Reset timeout
        clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_sec += 1;
      } else if (rc != 0) {
        // Handle unexpected errors
        fprintf(stderr, "pthread_cond_timedwait error: %d\n", rc);
        pthread_mutex_unlock(&kernel_mutex);
        return;
      }
    }
    pthread_mutex_unlock(&kernel_mutex);

    // Kernel is running, enter sampling loop
    while (true) {
      // Update correlation ID
      gpu_correlation_channel_receive(1, UpdateCorrelationID, NULL);
      
      // Wait for the event with a timeout
      ze_result_t status = zeEventHostSynchronize(event, 50000000 /* wait delay in nanoseconds */);
      PTI_ASSERT(status == ZE_RESULT_SUCCESS || status == ZE_RESULT_NOT_READY);
      
      if (status == ZE_RESULT_SUCCESS) {
        // Event is signaled, process it
        status = zeEventHostReset(event);
        PTI_ASSERT(status == ZE_RESULT_SUCCESS);
        CollectAndProcessMetrics(profiler, desc, streamer);
        streamer = FlushStreamerBuffer(streamer, desc, event, event_pool);
      }

      // Check if the kernel has finished
      pthread_mutex_lock(&kernel_mutex);
      bool is_kernel_finished = !kernel_running;
      pthread_mutex_unlock(&kernel_mutex);

      if (is_kernel_finished) {
        break;  // Exit sampling loop
      }
    }

    // Kernel has finished, perform final sampling and cleanup
    ze_result_t status = zeEventHostReset(event);
    PTI_ASSERT(status == ZE_RESULT_SUCCESS);
    CollectAndProcessMetrics(profiler, desc, streamer);
    streamer = FlushStreamerBuffer(streamer, desc, event, event_pool);
    
    // Notify the app thread that data processing is complete
    NotifyDataProcessingComplete();
  }
}

void 
ZeMetricProfiler::MetricProfilingThread
(
  ZeMetricProfiler* profiler, 
  ZeDeviceDescriptor *desc
) 
{
  ze_result_t status = ZE_RESULT_SUCCESS;

  ze_context_handle_t context = desc->context_;
  ze_device_handle_t device = desc->device_;
  zet_metric_group_handle_t group = desc->metric_group_;

  status = zetContextActivateMetricGroups(context, device, 1, &group);
  PTI_ASSERT(status == ZE_RESULT_SUCCESS);

  ze_event_pool_handle_t event_pool = nullptr;
  ze_event_pool_desc_t event_pool_desc = { ZE_STRUCTURE_TYPE_EVENT_POOL_DESC, nullptr, ZE_EVENT_POOL_FLAG_HOST_VISIBLE, 1 };
  status = zeEventPoolCreate(context, &event_pool_desc, 1, &device, &event_pool);
  PTI_ASSERT(status == ZE_RESULT_SUCCESS);

  ze_event_handle_t event = nullptr;
  ze_event_desc_t event_desc = { ZE_STRUCTURE_TYPE_EVENT_DESC, nullptr, 0, ZE_EVENT_SCOPE_FLAG_HOST, ZE_EVENT_SCOPE_FLAG_HOST };
  status = zeEventCreate(event_pool, &event_desc, &event);
  PTI_ASSERT(status == ZE_RESULT_SUCCESS);

  zet_metric_streamer_handle_t streamer = nullptr;
  uint32_t interval = 50 * 1000; // ns
  uint32_t notifyEveryNReports = 1024;

  zet_metric_streamer_desc_t streamer_desc = { ZET_STRUCTURE_TYPE_METRIC_STREAMER_DESC, nullptr, notifyEveryNReports, interval };
  status = zetMetricStreamerOpen(context, device, group, &streamer_desc, event, &streamer);
  if (status != ZE_RESULT_SUCCESS) {
    std::cerr << "[ERROR] Failed to open metric streamer (" << status << "). The sampling interval might be too small." << std::endl;

    status = zeEventDestroy(event);
    PTI_ASSERT(status == ZE_RESULT_SUCCESS);

    status = zeEventPoolDestroy(event_pool);
    PTI_ASSERT(status == ZE_RESULT_SUCCESS);

    // set state to enabled to let the parent thread continue
    desc->profiling_state_.store(PROFILER_ENABLED, std::memory_order_release);
    return;
  }

  if (streamer_desc.notifyEveryNReports > max_metric_samples) {
    max_metric_samples = streamer_desc.notifyEveryNReports;
  }

  std::vector<std::string> metrics_list;
  metrics_list = GetMetricList(group);
  PTI_ASSERT(!metrics_list.empty());

  RunProfilingLoop(profiler, desc, event, event_pool, streamer);

  status = zetMetricStreamerClose(streamer);
  PTI_ASSERT(status == ZE_RESULT_SUCCESS);

  status = zeEventDestroy(event);
  PTI_ASSERT(status == ZE_RESULT_SUCCESS);

  status = zeEventPoolDestroy(event_pool);
  PTI_ASSERT(status == ZE_RESULT_SUCCESS);

  status = zetContextActivateMetricGroups(context, device, 0, &group);
  PTI_ASSERT(status == ZE_RESULT_SUCCESS);
}

#endif // PTI_TOOLS_UNITRACE_LEVEL_ZERO_METRICS_H_