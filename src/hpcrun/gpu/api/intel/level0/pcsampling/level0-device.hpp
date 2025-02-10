// SPDX-FileCopyrightText: 2002-2024 Rice University
// SPDX-FileCopyrightText: 2024 Contributors to the HPCToolkit Project
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
#include <memory>
#include <thread>
#include <vector>


//*****************************************************************************
// local includes
//*****************************************************************************

#include "level0-assert.hpp"
#include "level0-context.hpp"
#include "level0-driver.hpp"
#include "level0-event.hpp"
#include "level0-eventpool.hpp"
#include "level0-metric.hpp"


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
  uint64_t correlation_id_;
  uint64_t last_correlation_id_;
  ze_kernel_handle_t running_kernel_;
  ze_event_handle_t running_kernel_end_;
  std::atomic<bool> kernel_started_{false};
  std::atomic<bool> serial_data_ready_{false};
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