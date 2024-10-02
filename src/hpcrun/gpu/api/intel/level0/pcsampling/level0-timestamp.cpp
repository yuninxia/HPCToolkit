// SPDX-FileCopyrightText: 2002-2024 Rice University
// SPDX-FileCopyrightText: 2024 Contributors to the HPCToolkit Project
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
zeroGetKernelExecutionTime
(
  ze_event_handle_t hSignalEvent,
  ze_device_handle_t hDevice
)
{
  KernelExecutionTime result = {0, 0, 0};
  
  if (hSignalEvent == nullptr) {
    return result;
  }

  ze_kernel_timestamp_result_t timestampResult;
  ze_result_t status = zeEventQueryKernelTimestamp(hSignalEvent, &timestampResult);
  level0_check_result(status, __LINE__);

  uint64_t startTime = timestampResult.global.kernelStart;
  uint64_t endTime = timestampResult.global.kernelEnd;
  uint64_t executionTime = endTime - startTime;

  ze_device_properties_t deviceProps = zeroGetDeviceProperties(hDevice);
  double timerResolution = deviceProps.timerResolution;

  result.startTimeNs = startTime * timerResolution;
  result.endTimeNs = endTime * timerResolution;
  result.executionTimeNs = executionTime * timerResolution;

  return result;
}
