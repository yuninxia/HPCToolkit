// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: Apache-2.0
//
// SPDX-License-Identifier: MIT
// Copyright (C) 2024 Intel Corporation
// This file was inspired by and uses some code fragments from Intel's
// MIT-licensed pti-gpu (https://github.com/intel/pti-gpu)

// -*-Mode: C++;-*-

//*****************************************************************************
// system includes
//*****************************************************************************

#include <inttypes.h>

//*****************************************************************************
// local includes
//*****************************************************************************

#include "level0-timestamp.hpp"
#include "level0-pc-api-receiver.hpp"


//******************************************************************************
// interface operations
//******************************************************************************

KernelExecutionTime
level0GetKernelExecutionTime
(
  ze_event_handle_t hSignalEvent,
  ze_device_handle_t hDevice,
  const struct hpcrun_foil_appdispatch_level0* dispatch
)
{
  KernelExecutionTime result = {0, 0, 0};

  if (hSignalEvent == nullptr) {
    pcsampling::warn("Null event handle passed to level0GetKernelExecutionTime");
    return result;
  }

  if (hDevice == nullptr) {
    pcsampling::warn("Null device handle passed to level0GetKernelExecutionTime");
    return result;
  }

  // Query the kernel timestamp
  ze_kernel_timestamp_result_t timestampResult;
  ze_result_t status = pcsampling::callZeEventQueryKernelTimestamp(hSignalEvent, &timestampResult, dispatch);
  level0_check_result(status, __LINE__);

  const uint64_t startTimestamp = timestampResult.global.kernelStart;
  const uint64_t endTimestamp   = timestampResult.global.kernelEnd;

  // Validate timestamps
  if (startTimestamp > endTimestamp) {
    pcsampling::warn("Invalid timestamps: start (%" PRIu64 ") is after end (%" PRIu64 ")",
                     startTimestamp, endTimestamp);
    return result;
  }

  const uint64_t kernelDuration = endTimestamp - startTimestamp;

  // Retrieve device properties to obtain the timer resolution
  ze_device_properties_t deviceProps = level0GetDeviceProperties(hDevice, dispatch);
  const double timerResolution = deviceProps.timerResolution;

  if (timerResolution <= 0) {
    pcsampling::warn("Invalid timer resolution: %f", timerResolution);
    return result;
  }

  // Convert timestamps to nanoseconds using the timer resolution
  result.startTimeNs     = startTimestamp * timerResolution;
  result.endTimeNs       = endTimestamp   * timerResolution;
  result.executionTimeNs = kernelDuration * timerResolution;

  return result;
}
