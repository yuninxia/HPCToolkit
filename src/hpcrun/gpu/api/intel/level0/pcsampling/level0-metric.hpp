// SPDX-FileCopyrightText: 2002-2024 Rice University
// SPDX-FileCopyrightText: 2024 Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: BSD-3-Clause

// -*-Mode: C++;-*-

#ifndef LEVEL0_METRIC_H
#define LEVEL0_METRIC_H

//*****************************************************************************
// level zero includes
//*****************************************************************************

#include <level_zero/zet_api.h>


//*****************************************************************************
// system includes
//*****************************************************************************

#include <cstdint>
#include <iostream>
#include <map>
#include <string>
#include <vector>


//*****************************************************************************
// local includes
//*****************************************************************************

#include "level0-assert.hpp"


//*****************************************************************************
// type definitions
//*****************************************************************************

struct EuStalls {
  uint64_t active_;
  uint64_t control_;
  uint64_t pipe_;
  uint64_t send_;
  uint64_t dist_;
  uint64_t sbid_;
  uint64_t sync_;
  uint64_t insfetch_;
  uint64_t other_;
};


//******************************************************************************
// interface operations
//******************************************************************************

void
zeroGetMetricGroup
(
  ze_device_handle_t device,
  const std::string& metric_group_name,
  zet_metric_group_handle_t& group
);

void
zeroMetricStreamerReadData
(
  zet_metric_streamer_handle_t streamer,
  std::vector<uint8_t>& storage,
  uint64_t& data_size
);

void
zeroMetricGroupCalculateMultipleMetricValuesExp
(
  zet_metric_group_handle_t metric_group,
  int raw_size,
  const std::vector<uint8_t>& raw_metrics,
  std::vector<uint32_t>& samples,
  std::vector<zet_typed_value_t>& metrics
);

void
zeroProcessMetrics
(
  const std::vector<std::string>& metric_list,
  const std::vector<uint32_t>& samples,
  const std::vector<zet_typed_value_t>& metrics,
  std::map<uint64_t, EuStalls>& eustalls
);


#endif  // LEVEL0_METRIC_H