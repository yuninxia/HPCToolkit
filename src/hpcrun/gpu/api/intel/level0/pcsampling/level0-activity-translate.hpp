// SPDX-FileCopyrightText: 2002-2024 Rice University
// SPDX-FileCopyrightText: 2024 Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: BSD-3-Clause

// -*-Mode: C++;-*-

#ifndef LEVEL0_ACTIVITY_TRANSLATE_H
#define LEVEL0_ACTIVITY_TRANSLATE_H

//*****************************************************************************
// system includes
//*****************************************************************************

#include <deque>
#include <iomanip>
#include <map>
#include <sstream>


//*****************************************************************************
// local includes
//*****************************************************************************

#include "../../../../activity/gpu-activity.h"
#include "../level0-id-map.h"
#include "level0-kernel-properties.hpp"
#include "level0-logging.hpp"
#include "level0-metric.hpp"


//******************************************************************************
// interface operations
//******************************************************************************

void
zeroActivityTranslate
(
  const std::map<uint64_t, EuStalls>::iterator& eustall_iter,               // [in] iterator to current EU stall entry (address -> stall info)
  const std::map<uint64_t, KernelProperties>::const_iterator& kernel_iter,  // [in] iterator to kernel properties entry (base address -> properties)
  uint64_t correlation_id,                                                  // [in] unique identifier to correlate related activities
  std::deque<gpu_activity_t*>& activities                                   // [out] queue for translated GPU activities
);


#endif // LEVEL0_ACTIVITY_TRANSLATE_H