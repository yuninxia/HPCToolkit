// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: BSD-3-Clause

// -*-Mode: C++;-*-

#ifndef LEVEL0_ACTIVITY_GENERATE_H
#define LEVEL0_ACTIVITY_GENERATE_H

//*****************************************************************************
// system includes
//*****************************************************************************

#include <algorithm>
#include <deque>
#include <unordered_map>


//******************************************************************************
// local includes
//******************************************************************************

#include "../../../../activity/gpu-activity.h"
#include "level0-activity-translate.hpp"
#include "level0-kernel-properties.hpp"
#include "level0-metric.hpp"
#include "level0-module.hpp"


//******************************************************************************
// interface operations
//******************************************************************************

void
level0GenerateActivities
(
  const std::map<uint64_t, KernelProperties>& kprops,    // [in] map from kernel base address to kernel properties
  std::map<uint64_t, EuStalls>& eustalls,                // [in] map from stall instruction address to EU stall information
  uint64_t& correlation_id,                              // [in] unique identifier for correlating kernel activities
  ze_kernel_handle_t running_kernel,                     // [in] handle to the currently running kernel
  std::deque<gpu_activity_t*>& activities,               // [out] queue for generated activities
  const struct hpcrun_foil_appdispatch_level0* dispatch  // [in] level0 dispatch interface
);


#endif // LEVEL0_ACTIVITY_GENERATE_H