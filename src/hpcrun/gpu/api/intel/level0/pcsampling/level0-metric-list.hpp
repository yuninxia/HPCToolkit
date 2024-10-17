// SPDX-FileCopyrightText: 2002-2024 Rice University
// SPDX-FileCopyrightText: 2024 Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: BSD-3-Clause

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

#include "level0-assert.hpp"


//******************************************************************************
// interface operations
//******************************************************************************

void
zeroGetMetricList
(
  zet_metric_group_handle_t group,
  std::vector<std::string>& name_list
);

bool
zeroIsValidMetricList
(
  const std::vector<std::string>& metric_list
);


#endif  // LEVEL0_METRIC_LIST_HPP