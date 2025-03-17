// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: BSD-3-Clause

// -*-Mode: C++;-*-

//*****************************************************************************
// local includes
//*****************************************************************************

#include "level0-timestamp.hpp"


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
    std::cerr << "[WARNING] Null event handle passed to level0GetKernelExecutionTime" << std::endl;
    return result;
  }

  if (hDevice == nullptr) {
    std::cerr << "[WARNING] Null device handle passed to level0GetKernelExecutionTime" << std::endl;
    return result;
  }

  // Query the kernel timestamp
  ze_kernel_timestamp_result_t timestampResult;
  ze_result_t status = f_zeEventQueryKernelTimestamp(hSignalEvent, &timestampResult, dispatch);
  level0_check_result(status, __LINE__);

  const uint64_t startTimestamp = timestampResult.global.kernelStart;
  const uint64_t endTimestamp   = timestampResult.global.kernelEnd;
  
  // Validate timestamps
  if (startTimestamp > endTimestamp) {
    std::cerr << "[WARNING] Invalid timestamps: start (" << startTimestamp 
              << ") is after end (" << endTimestamp << ")" << std::endl;
    return result;
  }
  
  const uint64_t kernelDuration = endTimestamp - startTimestamp;

  // Retrieve device properties to obtain the timer resolution
  try {
    ze_device_properties_t deviceProps = level0GetDeviceProperties(hDevice, dispatch);
    const double timerResolution = deviceProps.timerResolution;

    if (timerResolution <= 0) {
      std::cerr << "[WARNING] Invalid timer resolution: " << timerResolution << std::endl;
      return result;
    }

    // Convert timestamps to nanoseconds using the timer resolution
    result.startTimeNs     = startTimestamp * timerResolution;
    result.endTimeNs       = endTimestamp   * timerResolution;
    result.executionTimeNs = kernelDuration * timerResolution;
  } catch (const std::exception& e) {
    std::cerr << "[ERROR] Exception in level0GetKernelExecutionTime: " << e.what() << std::endl;
  }

  return result;
}
