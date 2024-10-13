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
  std::deque<gpu_activity_t*>& activities, 
  const std::map<uint64_t, EuStalls>::iterator& eustall_iter,
  const std::map<uint64_t, KernelProperties>::const_reverse_iterator& kernel_iter,
  uint64_t correlation_id
);


#endif // LEVEL0_ACTIVITY_TRANSLATE_H