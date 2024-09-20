// SPDX-FileCopyrightText: 2002-2024 Rice University
// SPDX-FileCopyrightText: 2024 Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: BSD-3-Clause

// -*-Mode: C++;-*-

#ifndef LEVEL0_ACTIVITY_GENERATE_H
#define LEVEL0_ACTIVITY_GENERATE_H

//*****************************************************************************
// system includes
//*****************************************************************************

#include <deque>
#include <map>
#include <unordered_map>


//******************************************************************************
// local variables
//******************************************************************************

#include "../../../../activity/gpu-activity.h"
#include "level0-activity-translate.h"
#include "level0-kernel-properties.h"
#include "level0-metric.h"


//******************************************************************************
// interface operations
//******************************************************************************

void
zeroGenerateActivities
(
  const std::map<uint64_t, KernelProperties>& kprops, 
  std::map<uint64_t, EuStalls>& eustalls,
  uint64_t& correlation_id,
  std::deque<gpu_activity_t*>& activities,
  ze_kernel_handle_t running_kernel
);


#endif // LEVEL0_ACTIVITY_GENERATE_H