// SPDX-FileCopyrightText: 2002-2024 Rice University
// SPDX-FileCopyrightText: 2024 Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: BSD-3-Clause

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
#include <unordered_map>


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
zeroLogActivities
(
  const std::deque<gpu_activity_t*>& activities, 
  const std::map<uint64_t, KernelProperties>& kprops
);

void
zeroLogPCSample
(
  uint64_t correlation_id,
  const KernelProperties& kernel_props,
  const gpu_pc_sampling_t& pc_sampling,
  const EuStalls& stall,
  uint64_t base_address
);


#endif // LEVEL0_LOGGING_HPP