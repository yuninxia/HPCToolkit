// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: Apache-2.0
//
// SPDX-License-Identifier: MIT
// Copyright (C) 2024 Intel Corporation
// This file was inspired by and uses some code fragments from Intel's
// MIT-licensed pti-gpu (https://github.com/intel/pti-gpu)

// -*-Mode: C++;-*-

#ifndef LEVEL0_TIMESTAMP_HPP
#define LEVEL0_TIMESTAMP_HPP

//*****************************************************************************
// level zero includes
//*****************************************************************************

#include <level_zero/ze_api.h>


//*****************************************************************************
// local includes
//*****************************************************************************

#include "level0-assert.hpp"
#include "level0-device.hpp"


//*****************************************************************************
// type definitions
//*****************************************************************************

struct KernelExecutionTime {
    double startTimeNs;
    double endTimeNs;
    double executionTimeNs;
};


//******************************************************************************
// interface operations
//******************************************************************************

KernelExecutionTime
level0GetKernelExecutionTime
(
  ze_event_handle_t hSignalEvent,
  ze_device_handle_t hDevice,
  const struct hpcrun_foil_appdispatch_level0* dispatch
);


#endif // LEVEL0_TIMESTAMP_HPP
