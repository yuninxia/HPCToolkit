// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: Apache-2.0
//
// SPDX-License-Identifier: MIT
// Copyright (C) 2024 Intel Corporation
// This file was inspired by and uses some code fragments from Intel's
// MIT-licensed pti-gpu (https://github.com/intel/pti-gpu)

// -*-Mode: C++;-*-

#ifndef LEVEL0_METRIC_LIST_HPP
#define LEVEL0_METRIC_LIST_HPP

//*****************************************************************************
// level zero includes
//*****************************************************************************

#include <level_zero/zet_api.h>


//*****************************************************************************
// system includes
//*****************************************************************************

#include <string>
#include <vector>


//*****************************************************************************
// local includes
//*****************************************************************************

#include "../../../../../foil/level0.h"
#include "level0-assert.hpp"


//******************************************************************************
// interface operations
//******************************************************************************

void
level0GetMetricList
(
  zet_metric_group_handle_t group,
  std::vector<std::string>& name_list,
  const struct hpcrun_foil_appdispatch_level0* dispatch
);

bool
level0IsValidMetricList
(
  const std::vector<std::string>& metric_list
);


#endif  // LEVEL0_METRIC_LIST_HPP
