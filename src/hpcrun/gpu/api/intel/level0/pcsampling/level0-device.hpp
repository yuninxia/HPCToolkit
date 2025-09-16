// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: BSD-3-Clause

// -*-Mode: C++;-*-

#ifndef LEVEL0_DEVICE_H
#define LEVEL0_DEVICE_H

//*****************************************************************************
// level zero includes
//*****************************************************************************

#include <level_zero/ze_api.h>
#include <level_zero/zet_api.h>


//*****************************************************************************
// system includes
//*****************************************************************************

#include <atomic>
#include <thread>


//*****************************************************************************
// local includes
//*****************************************************************************

#include "level0-assert.hpp"
#include "level0-context.hpp"
#include "level0-driver.hpp"
#include "level0-event.hpp"
#include "level0-eventpool.hpp"
#include "level0-metric.hpp"

extern "C" {
  #include "../../../../../../common/lean/mcs-lock.h"
}

//*****************************************************************************
// type definitions
//*****************************************************************************

enum ZeProfilerState {
  PROFILER_UNKNOWN = -1,
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
  std::atomic<int> profiling_state_{PROFILER_UNKNOWN};
  bool stall_sampling_;

  uint64_t correlation_id_;
  ze_kernel_handle_t running_kernel_;
  ze_event_handle_t running_kernel_end_;

  std::atomic<bool> kernel_started_{false};
  std::atomic<bool> serial_data_ready_{false};
  mcs_lock_t kernel_launch_lock;

  // FIXME(Yuning)
  // application_UpdateProfilerState
  // profiler_XXX

  void UpdateProfilerState(int state) {
    profiling_state_.store(state, std::memory_order_release);
  }

  bool IsProfilerActive() const {
    return profiling_state_.load(std::memory_order_acquire) != PROFILER_DISABLED;
  }

  bool IsProfilerDisabled() const {
    return profiling_state_.load(std::memory_order_acquire) == PROFILER_DISABLED;
  }

  bool IsProfilerInitialized() const { // Check if specifically set to ENABLED
    return profiling_state_.load(std::memory_order_acquire) == PROFILER_ENABLED;
  }

  void SetKernelStarted(bool started) {
    kernel_started_.store(started, std::memory_order_release);
  }

  bool IsKernelStarted() const {
    return kernel_started_.load(std::memory_order_acquire);
  }

  void SetSerialDataReady(bool ready) {
    serial_data_ready_.store(ready, std::memory_order_release);
  }

  bool IsSerialDataReady() const {
    return serial_data_ready_.load(std::memory_order_acquire);
  }
};

struct ZeDevice {
  ze_device_handle_t device_;
  ze_device_handle_t parent_device_;
  ze_driver_handle_t driver_;
  int32_t id_;
  int32_t parent_id_;
  int32_t subdevice_id_;
  int32_t num_subdevices_;
};


//******************************************************************************
// global variables
//******************************************************************************

extern std::map<ze_device_handle_t, ZeDevice> *devices_;


//******************************************************************************
// interface operations
//******************************************************************************

std::vector<ze_device_handle_t>
level0GetDevices
(
  ze_driver_handle_t driver,
  const struct hpcrun_foil_appdispatch_level0* dispatch
);

std::vector<ze_device_handle_t>
level0GetSubDevices
(
  ze_device_handle_t device,
  uint32_t num_sub_devices,
  const struct hpcrun_foil_appdispatch_level0* dispatch
);

uint32_t
level0GetSubDeviceCount
(
  ze_device_handle_t device,
  const struct hpcrun_foil_appdispatch_level0* dispatch
);

void 
level0EnumerateDevices
(
  std::map<ze_device_handle_t, ZeDeviceDescriptor*>& device_descriptors_,
  std::vector<ze_context_handle_t>& metric_contexts,
  const struct hpcrun_foil_appdispatch_level0* dispatch
);

ze_device_properties_t
level0GetDeviceProperties
(
  ze_device_handle_t device,
  const struct hpcrun_foil_appdispatch_level0* dispatch
);

ze_device_handle_t
level0DeviceGetRootDevice
(
  ze_device_handle_t device,
  const struct hpcrun_foil_appdispatch_level0* dispatch
);

void
level0EnumerateAndSetupDevices
(
  const struct hpcrun_foil_appdispatch_level0* dispatch
);


#endif  // LEVEL0_DEVICE_H