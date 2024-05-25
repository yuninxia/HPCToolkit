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
#include "pti_assert.h"
#include "ze_utils.h"

//*****************************************************************************
// macros
//*****************************************************************************

constexpr static uint32_t max_metric_size = 512;
static uint32_t max_metric_samples = 32768;

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
  std::string metric_file_name_;
  std::ofstream metric_file_stream_;
  std::vector<uint8_t> metric_data_;
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
};

//*****************************************************************************
// class definition
//*****************************************************************************
class ZeMetricProfiler {
 public:
  static ZeMetricProfiler* Create(char *dir) {
    ZeMetricProfiler* profiler = new ZeMetricProfiler(dir);
    profiler->StartProfilingMetrics();
    return profiler;
  }

  ~ZeMetricProfiler() {
    StopProfilingMetrics();
    ComputeMetrics();
  }

  ZeMetricProfiler(const ZeMetricProfiler& that) = delete;
  ZeMetricProfiler& operator=(const ZeMetricProfiler& that) = delete;

 private:
  ZeMetricProfiler(char *dir) {
    data_dir_name_ = std::string(dir);
    EnumerateDevices(dir);
  }

  void StartProfilingMetrics() {
    for (auto it = device_descriptors_.begin(); it != device_descriptors_.end(); ++it) {
      if (it->second->parent_device_ != nullptr) {
        // subdevice
        continue;
      }
      monitor_disable_new_threads();
      it->second->profiling_thread_ = new std::thread(MetricProfilingThread, it->second);
      monitor_enable_new_threads();
      while (it->second->profiling_state_.load(std::memory_order_acquire) != PROFILER_ENABLED) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
      }
    }
  }

  void StopProfilingMetrics() {
    for (auto it = device_descriptors_.begin(); it != device_descriptors_.end(); ++it) {
      if (it->second->parent_device_ != nullptr) {
        // subdevice
        continue;
      }
      PTI_ASSERT(it->second->profiling_thread_ != nullptr);
      PTI_ASSERT(it->second->profiling_state_ == PROFILER_ENABLED);
      it->second->profiling_state_.store(PROFILER_DISABLED, std::memory_order_release);
      it->second->profiling_thread_->join();
      delete it->second->profiling_thread_;
      it->second->profiling_thread_ = nullptr;
      it->second->metric_file_stream_.close();
    }
  }

  void EnumerateDevices(char *dir) {
    std::string metric_group = "EuStallSampling";
    bool stall_sampling = (metric_group == "EuStallSampling");

    ze_result_t status = ZE_RESULT_SUCCESS;
    uint32_t num_drivers = 0;
    status = zeDriverGet(&num_drivers, nullptr);
    PTI_ASSERT(status == ZE_RESULT_SUCCESS);
    if (num_drivers > 0) {
      int32_t did = 0;
      std::vector<ze_driver_handle_t> drivers(num_drivers);
      status = zeDriverGet(&num_drivers, drivers.data());
      PTI_ASSERT(status == ZE_RESULT_SUCCESS);
      for (auto driver : drivers) {
        ze_context_handle_t context = nullptr;
        ze_context_desc_t cdesc = {ZE_STRUCTURE_TYPE_CONTEXT_DESC, nullptr, 0};

        status = zeContextCreate(driver, &cdesc, &context);
        PTI_ASSERT(status == ZE_RESULT_SUCCESS);
        metric_contexts_.push_back(context);

        uint32_t num_devices = 0;
        status = zeDeviceGet(driver, &num_devices, nullptr);
        PTI_ASSERT(status == ZE_RESULT_SUCCESS);
        if (num_devices > 0) {
          std::vector<ze_device_handle_t> devices(num_devices);
          status = zeDeviceGet(driver, &num_devices, devices.data());
          PTI_ASSERT(status == ZE_RESULT_SUCCESS);
          for (auto device : devices) {
            uint32_t num_sub_devices = 0;
            status = zeDeviceGetSubDevices(device, &num_sub_devices, nullptr);
            PTI_ASSERT(status == ZE_RESULT_SUCCESS);

            ZeDeviceDescriptor *desc = new ZeDeviceDescriptor;

            desc->stall_sampling_ = stall_sampling;
            desc->device_ = device;
            desc->device_id_ = did;
            desc->parent_device_id_ = -1;    // no parent device
            desc->parent_device_ = nullptr;
            desc->subdevice_id_ = -1;        // not a subdevice
            desc->num_sub_devices_ = num_sub_devices;

            zet_metric_group_handle_t group = nullptr;
            uint32_t num_groups = 0;
            status = zetMetricGroupGet(device, &num_groups, nullptr);
            PTI_ASSERT(status == ZE_RESULT_SUCCESS);
            if (num_groups > 0) {
              std::vector<zet_metric_group_handle_t> groups(num_groups, nullptr);
              status = zetMetricGroupGet(device, &num_groups, groups.data());
              PTI_ASSERT(status == ZE_RESULT_SUCCESS);

              for (uint32_t k = 0; k < num_groups; ++k) {
                zet_metric_group_properties_t group_props{};
                group_props.stype = ZET_STRUCTURE_TYPE_METRIC_GROUP_PROPERTIES;
                status = zetMetricGroupGetProperties(groups[k], &group_props);
                PTI_ASSERT(status == ZE_RESULT_SUCCESS);

                if ((strcmp(group_props.name, metric_group.c_str()) == 0) && (group_props.samplingType & ZET_METRIC_GROUP_SAMPLING_TYPE_FLAG_TIME_BASED)) {
                  group = groups[k];
                  break;
                }
              }
            }

            if (group == nullptr) {
              std::cerr << "[ERROR] Invalid metric group " << metric_group << std::endl;
              exit(-1);
            }

            desc->driver_ = driver;
            desc->context_ = context;
            desc->metric_group_ = group;
            desc->profiling_thread_ = nullptr;
            desc->profiling_state_.store(PROFILER_DISABLED, std::memory_order_release);
            desc->metric_file_name_ = std::string(dir) + "/." + std::to_string(did) + "." + metric_group + "." + std::to_string(getpid()) + ".t";
            desc->metric_file_stream_ = std::ofstream(desc->metric_file_name_, std::ios::out | std::ios::trunc | std::ios::binary);
            device_descriptors_.insert({device, desc});

            if (num_sub_devices > 0) {
              std::vector<ze_device_handle_t> sub_devices(num_sub_devices);
              status = zeDeviceGetSubDevices(device, &num_sub_devices, sub_devices.data());
              PTI_ASSERT(status == ZE_RESULT_SUCCESS);

              for (uint32_t j = 0; j < num_sub_devices; j++) {
                ZeDeviceDescriptor *sub_desc = new ZeDeviceDescriptor;
                sub_desc->stall_sampling_ = stall_sampling;
                sub_desc->device_ = sub_devices[j];
                sub_desc->device_id_ = did;        // subdevice
                sub_desc->parent_device_id_ = did;    // parent device
                sub_desc->parent_device_ = device;
                sub_desc->subdevice_id_ = j;        // a subdevice
                sub_desc->num_sub_devices_ = 0;
                sub_desc->driver_ = driver;
                sub_desc->context_ = context;
                sub_desc->metric_group_ = group;
                sub_desc->driver_ = driver;
                sub_desc->context_ = context;
                sub_desc->metric_group_ = group;
                sub_desc->profiling_thread_ = nullptr;
                sub_desc->profiling_state_.store(PROFILER_DISABLED, std::memory_order_release);

                device_descriptors_.insert({sub_devices[j], sub_desc});
              }
            }
            did++;
          }
        }
      }
    }
  }

  int GetDeviceId(ze_device_handle_t sub_device) const {
    auto it = device_descriptors_.find(sub_device);
    if (it != device_descriptors_.end()) {
      return it->second->device_id_;
    }
    return -1;
  }

  int GetSubDeviceId(ze_device_handle_t sub_device) const {
    auto it = device_descriptors_.find(sub_device);
    if (it != device_descriptors_.end()) {
      return it->second->subdevice_id_;
    }
    return -1;
  }

  ze_device_handle_t GetParentDevice(ze_device_handle_t sub_device) const {
    auto it = device_descriptors_.find(sub_device);
    if (it != device_descriptors_.end()) {
      return it->second->parent_device_;
    }
    return nullptr;
  }

  static bool level0_convert_pcsampling(gpu_activity_t* activity, uint64_t offset, const EuStalls& stall, uint64_t correlation_id) {
    activity->kind = GPU_ACTIVITY_PC_SAMPLING;

    activity->details.pc_sampling.correlation_id = correlation_id;

    ip_normalized_t normalized_ip;
    normalized_ip.lm_id = 0;
    normalized_ip.lm_ip = offset;
    activity->details.pc_sampling.pc = normalized_ip;

    activity->details.pc_sampling.samples = stall.active_ + stall.control_ + stall.pipe_ + stall.send_ + stall.dist_ + stall.sbid_ + stall.sync_ + stall.insfetch_ + stall.other_;

    activity->details.pc_sampling.latencySamples = activity->details.pc_sampling.samples - stall.active_;

    activity->details.pc_sampling.stallReason = level0_convert_stall_reason(stall);

    return true;
  }

  static void level0_activity_translate(std::deque<gpu_activity_t*>& activities, uint64_t offset, const EuStalls& stall, uint64_t correlation_id) {
    gpu_activity_t* activity = new gpu_activity_t();
    gpu_activity_init(activity);

    if (level0_convert_pcsampling(activity, offset, stall, correlation_id)) {
      activities.push_back(activity);
    } else {
      delete activity;
    }
  }

  static gpu_inst_stall_t level0_convert_stall_reason(const EuStalls& stall) {
    gpu_inst_stall_t stall_reason = GPU_INST_STALL_NONE;
    uint64_t max_value = 0;

    if (stall.control_ > max_value) {
      max_value = stall.control_;
      stall_reason = GPU_INST_STALL_PIPE_BUSY; // TBD
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
      stall_reason = GPU_INST_STALL_TMEM; // TBD
    }
    if (stall.sbid_ > max_value) {
      max_value = stall.sbid_;
      stall_reason = GPU_INST_STALL_SYNC; // TBD
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

  static uint64_t level0_generate_correlation_id(const std::string& kernel_id, const std::string& module_id) { 
    uint32_t kernel_id_uint32 = 0, module_id_uint32 = 0;
    std::stringstream ss;

    ss << std::hex << kernel_id.substr(0, 8);
    ss >> kernel_id_uint32;
    ss.clear();
    ss.str("");
    
    ss << std::hex << module_id.substr(0, 8);
    ss >> module_id_uint32;

    return ((uint64_t)module_id_uint32 << 32) | (uint64_t)kernel_id_uint32;
  }

  void ComputeMetrics() {
    std::deque<gpu_activity_t*> activities;

    for (auto it = device_descriptors_.begin(); it != device_descriptors_.end(); ++it) {
      if (it->second->parent_device_ != nullptr) {
        // subdevice
        continue;
      }

      if (it->second->stall_sampling_) {
        std::map<uint64_t, KernelProperties> kprops;
        int max_kname_size = 0;
        // enumerate all kernel property files
        for (const auto& e : std::filesystem::directory_iterator(std::filesystem::path(data_dir_name_))) {
          // kernel properties file path: <data_dir>/.kprops.<device_id>.<pid>.txt
          if (e.path().filename().string().find(".kprops." + std::to_string(it->second->device_id_)) == 0) {
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

              if (props.name.size() > static_cast<std::string::size_type>(max_kname_size)) {
                max_kname_size = props.name.size();
              }
              kprops.insert({props.base_address, std::move(props)});
            }
            kpf.close();
          }
        }
        if (kprops.size() == 0) {
          continue;
        }

        std::map<uint64_t, EuStalls> eustalls;

        std::vector<std::string> metric_list;
        metric_list = GetMetricList(it->second->metric_group_);
        PTI_ASSERT(!metric_list.empty());

        uint32_t ip_idx = GetMetricId(metric_list, "IP");
        if (ip_idx >= metric_list.size()) {
          // no IP metric 
          continue;
        }

        std::ifstream inf = std::ifstream(it->second->metric_file_name_, std::ios::in | std::ios::binary);
        std::vector<uint8_t> raw_metrics(MAX_METRIC_BUFFER + 512);

        if (!inf.is_open()) {
          continue;
        }

        while (!inf.eof()) {
          inf.read(reinterpret_cast<char *>(raw_metrics.data()), MAX_METRIC_BUFFER + 512);
          int raw_size = inf.gcount();
          if (raw_size > 0) {
            uint32_t num_samples = 0;
            uint32_t num_metrics = 0;
            ze_result_t status = zetMetricGroupCalculateMultipleMetricValuesExp(
              it->second->metric_group_, ZET_METRIC_GROUP_CALCULATION_TYPE_METRIC_VALUES,
              raw_size, raw_metrics.data(), &num_samples, &num_metrics,
              nullptr, nullptr);
            if ((status != ZE_RESULT_SUCCESS) || (num_samples == 0) || (num_metrics == 0)) {
              std::cerr << "[WARNING] Unable to calculate metrics" << std::endl;
              continue;
            }

            std::vector<uint32_t> samples(num_samples);
            std::vector<zet_typed_value_t> metrics(num_metrics);

            status = zetMetricGroupCalculateMultipleMetricValuesExp(
              it->second->metric_group_, ZET_METRIC_GROUP_CALCULATION_TYPE_METRIC_VALUES,
              raw_size, raw_metrics.data(), &num_samples, &num_metrics,
              samples.data(), metrics.data());

            if ((status != ZE_RESULT_SUCCESS) && (status != ZE_RESULT_WARNING_DROPPED_DATA)) {
              std::cerr << "[WARNING] Unable to calculate metrics" << std::endl;
              continue;
            }

            const zet_typed_value_t *value = metrics.data();
            for (uint32_t i = 0; i < num_samples; ++i) {
              uint32_t size = samples[i];

              for (uint32_t j = 0; j < size; j += metric_list.size()) {
                uint64_t ip;
                ip = (value[j + 0].value.ui64 << 3);
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
                auto eit = eustalls.find(ip);
                if (eit != eustalls.end()) {
                  eit->second.active_ += stall.active_;
                  eit->second.control_ += stall.control_;
                  eit->second.pipe_ += stall.pipe_;
                  eit->second.send_ += stall.send_;
                  eit->second.dist_ += stall.dist_;
                  eit->second.sbid_ += stall.sbid_;
                  eit->second.sync_ += stall.sync_;
                  eit->second.insfetch_ += stall.insfetch_;
                  eit->second.other_ += stall.other_;
                } else {
                  eustalls.insert({ip, std::move(stall)});
                }
              }
              value += samples[i];
            }
          }
        }
        inf.close();

        if (eustalls.size() == 0) {
          continue;
        }

        for (auto it = eustalls.begin(); it != eustalls.end(); ++it) {
          for (auto rit = kprops.crbegin(); rit != kprops.crend(); ++rit) {
            if ((rit->first <= it->first) && ((it->first - rit->first) < rit->second.size)) {
              uint64_t offset = it->first - rit->first;
              uint64_t correlation_id = level0_generate_correlation_id(rit->second.kernel_id, rit->second.module_id);
              level0_activity_translate(activities, offset, it->second, correlation_id);
              break;
            }
          }
        }
      }
    }

    for (auto activity : activities) {
      delete activity;
    }
  }

 private:

  inline static std::string GetMetricUnits(const char* units) {
    PTI_ASSERT(units != nullptr);

    std::string result = units;
    if (result.find("null") != std::string::npos) {
      result = "";
    } else if (result.find("percent") != std::string::npos) {
      result = "%";
    }

    return result;
  }

  inline uint32_t GetMetricId(const std::vector<std::string>& metric_list, const std::string& metric_name) {
    PTI_ASSERT(!metric_list.empty());
    PTI_ASSERT(!metric_name.empty());

    for (size_t i = 0; i < metric_list.size(); ++i) {
      if (metric_list[i].find(metric_name) == 0) {
        return i;
      }
    }

    return metric_list.size();
  }

  static uint32_t GetMetricCount(zet_metric_group_handle_t group) {
    PTI_ASSERT(group != nullptr);

    zet_metric_group_properties_t group_props{};
    group_props.stype = ZET_STRUCTURE_TYPE_METRIC_GROUP_PROPERTIES;
    ze_result_t status = zetMetricGroupGetProperties(group, &group_props);
    PTI_ASSERT(status == ZE_RESULT_SUCCESS);

    return group_props.metricCount;
  }

  static std::vector<std::string> GetMetricList(zet_metric_group_handle_t group) {
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

  static uint64_t ReadMetrics(ze_event_handle_t event, zet_metric_streamer_handle_t streamer, std::vector<uint8_t>& storage) {
    ze_result_t status = ZE_RESULT_SUCCESS;

    status = zeEventQueryStatus(event);
    PTI_ASSERT(status == ZE_RESULT_SUCCESS || status == ZE_RESULT_NOT_READY);
    if (status == ZE_RESULT_SUCCESS) {
      status = zeEventHostReset(event);
      PTI_ASSERT(status == ZE_RESULT_SUCCESS);
    } else {
      return 0;
    }

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

  static void MetricProfilingThread(ZeDeviceDescriptor *desc) {
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
    uint32_t interval = 50 * 1000;    // convert us to ns

    zet_metric_streamer_desc_t streamer_desc = { ZET_STRUCTURE_TYPE_METRIC_STREAMER_DESC, nullptr, max_metric_samples, interval };
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

    std::vector<uint8_t> raw_metrics(MAX_METRIC_BUFFER + 512);

    auto it = raw_metrics.begin();
    desc->profiling_state_.store(PROFILER_ENABLED, std::memory_order_release);
    while (desc->profiling_state_.load(std::memory_order_acquire) != PROFILER_DISABLED) {
      uint64_t size = ReadMetrics(event, streamer, raw_metrics);
      if (size == 0) {
        if (!desc->metric_data_.empty()) {
          desc->metric_file_stream_.write(reinterpret_cast<char*>(desc->metric_data_.data()), desc->metric_data_.size());
          desc->metric_data_.clear();
        }
        continue;
      }
      desc->metric_data_.insert(desc->metric_data_.end(), it, it + size);
    }
    auto size = ReadMetrics(event, streamer, raw_metrics);
    desc->metric_data_.insert(desc->metric_data_.end(), it, it + size);
    if (!desc->metric_data_.empty()) {
      desc->metric_file_stream_.write(reinterpret_cast<char*>(desc->metric_data_.data()), desc->metric_data_.size());
      desc->metric_data_.clear();
    }
    raw_metrics.clear();

    status = zetMetricStreamerClose(streamer);
    PTI_ASSERT(status == ZE_RESULT_SUCCESS);

    status = zeEventDestroy(event);
    PTI_ASSERT(status == ZE_RESULT_SUCCESS);

    status = zeEventPoolDestroy(event_pool);
    PTI_ASSERT(status == ZE_RESULT_SUCCESS);

    status = zetContextActivateMetricGroups(context, device, 0, &group);
    PTI_ASSERT(status == ZE_RESULT_SUCCESS);
  }

 private: // Data

  std::vector<ze_context_handle_t> metric_contexts_;
  std::map<ze_device_handle_t, ZeDeviceDescriptor *> device_descriptors_;
  std::string data_dir_name_;
};

#endif // PTI_TOOLS_UNITRACE_LEVEL_ZERO_METRICS_H_

