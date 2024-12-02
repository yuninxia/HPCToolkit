// SPDX-FileCopyrightText: 2002-2024 Rice University
// SPDX-FileCopyrightText: 2024 Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: BSD-3-Clause

// -*-Mode: C++;-*-

#ifndef LEVEL0_COLLECTOR_H_
#define LEVEL0_COLLECTOR_H_

//*****************************************************************************
// level zero includes
//*****************************************************************************

#include <level_zero/ze_api.h>
#include <level_zero/zet_api.h>


//*****************************************************************************
// system includes
//*****************************************************************************

#include <map>
#include <string>
#include <unordered_map>
#include <vector>


//******************************************************************************
// local includes
//******************************************************************************

#include "level0-assert.hpp"
#include "level0-device.hpp"
#include "level0-driver.hpp"
#include "level0-logging.hpp"
#include "level0-metric-profiler.hpp"
#include "level0-module.hpp"
#include "level0-tracing-callback-methods.hpp"
#include "level0-tracing.hpp"


//*****************************************************************************
// class definition
//*****************************************************************************

class ZeCollector {
 public:
  static ZeCollector* Create(const std::string& data_dir, const struct hpcrun_foil_appdispatch_level0* dispatch);
  ZeCollector(const ZeCollector& that) = delete;
  ZeCollector& operator=(const ZeCollector& that) = delete;
  ~ZeCollector();

  std::string GetDataDir() const { return data_dir_; }
  const struct hpcrun_foil_appdispatch_level0* getDispatch() const { return dispatch_; }

 private:
  ZeCollector(const std::string& data_dir, const struct hpcrun_foil_appdispatch_level0* dispatch);

 private:
  std::string data_dir_;
  const struct hpcrun_foil_appdispatch_level0* dispatch_;
};


#endif // LEVEL0_COLLECTOR_H_