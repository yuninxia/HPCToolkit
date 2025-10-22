// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: Apache-2.0
//
// SPDX-License-Identifier: MIT
// Copyright (C) 2024 Intel Corporation
// This file was inspired by and uses some code fragments from Intel's
// MIT-licensed pti-gpu (https://github.com/intel/pti-gpu)

// -*-Mode: C++;-*-

#ifndef LEVEL0_LOGGING_HPP
#define LEVEL0_LOGGING_HPP

//*****************************************************************************
// level zero includes
//*****************************************************************************

#include <level_zero/ze_api.h>


//*****************************************************************************
// system includes
//*****************************************************************************

#include <deque>
#include <map>
#include <string>
#include <unordered_map>
#include <vector>


//*****************************************************************************
// local includes
//*****************************************************************************

#include "../../../../activity/gpu-activity.h"
#include "../level0-id-map.h"
#include "level0-assert.hpp"
#include "level0-kernel-properties.hpp"
#include "level0-metric.hpp"


//******************************************************************************
// interface operations
//******************************************************************************

void
level0LogActivities
(
  const std::deque<gpu_activity_t*>& activities,
  const std::map<uint64_t, KernelProperties>& kprops
);

void
level0LogPCSample
(
  uint64_t correlation_id,
  const KernelProperties& kernel_props,
  const gpu_pc_sampling_t& pc_sampling,
  const EuStalls& stall,
  uint64_t base_address
);

void
level0LogMetricList
(
  const std::vector<std::string>& metric_list
);

void
level0LogSamplesAndMetrics
(
  const std::vector<uint32_t>& samples,
  const std::vector<zet_typed_value_t>& metrics
);


#endif // LEVEL0_LOGGING_HPP
