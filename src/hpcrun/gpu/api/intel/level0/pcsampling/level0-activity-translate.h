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
#include <string>


//*****************************************************************************
// local includes
//*****************************************************************************

#include "../../../../activity/gpu-activity.h"
#include "../level0-id-map.h"
#include "level0-kernel-properties.h"
#include "level0-metric.h"


//******************************************************************************
// interface operations
//******************************************************************************

void
zeroActivityTranslate
(
  std::deque<gpu_activity_t*>& activities, 
  const std::map<uint64_t, EuStalls>::iterator& it,
  const std::map<uint64_t, KernelProperties>::const_reverse_iterator& rit,
  uint64_t correlation_id
);

template <typename T>
T hex_string_to_uint
(
  const std::string& hex_str
) 
{
  std::stringstream ss;
  T num = 0;
  ss << std::hex << hex_str;
  ss >> num;
  return num;
}


#endif // LEVEL0_ACTIVITY_TRANSLATE_H