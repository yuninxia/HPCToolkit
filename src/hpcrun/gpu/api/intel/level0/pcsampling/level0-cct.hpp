// SPDX-FileCopyrightText: 2002-2024 Rice University
// SPDX-FileCopyrightText: 2024 Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: BSD-3-Clause

// -*-Mode: C++;-*-

#ifndef LEVEL0_CCT_H
#define LEVEL0_CCT_H

//*****************************************************************************
// system includes
//*****************************************************************************

#include <deque>
#include <iostream>


//*****************************************************************************
// local includes
//*****************************************************************************

#include "../../../../../../common/lean/OSUtil.h"
#include "../../../../activity/gpu-op-placeholders.h" // need extern "C"
#include "level0-kernel-properties.hpp"

extern "C" {
  #include "../../../../gpu-metrics.h"
  #include "../../../../activity/gpu-activity-process.h"
  #include "../../../../../write_data.h"
  #include "../../../../../thread_data.h"
  #include "../../../../../safe-sampling.h"
}


//******************************************************************************
// interface operations
//******************************************************************************

void
zeroInitIdTuple
(
  thread_data_t* td,
  uint32_t device_id,
  int thread_id
);

thread_data_t* 
zeroInitThreadData
(
  int thread_id,
  bool demand_new_thread
);

bool
zeroBuildCCT
(
  thread_data_t* td, 
  std::deque<gpu_activity_t*>& activities
);


#endif // LEVEL0_CCT_H