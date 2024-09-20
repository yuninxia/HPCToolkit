// SPDX-FileCopyrightText: 2002-2024 Rice University
// SPDX-FileCopyrightText: 2024 Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: BSD-3-Clause

// -*-Mode: C++;-*-

//*****************************************************************************
// system includes
//*****************************************************************************

#include <iostream>
#include <string>


//*****************************************************************************
// local includes
//*****************************************************************************

#include "level0-tracing-callbacks.h"

class UniTracer {
 public:
  static UniTracer* Create(const std::string& data_dir_name);
  ~UniTracer();
  UniTracer(const UniTracer& that) = delete;
  UniTracer& operator=(const UniTracer& that) = delete;

 private:
  UniTracer();

 private:
  ZeCollector* ze_collector_ = nullptr;
};

UniTracer* 
UniTracer::Create
(
  const std::string& data_dir_name
) 
{
  UniTracer* tracer = new UniTracer();
  ZeCollector* ze_collector = nullptr;
  ze_collector = ZeCollector::Create(data_dir_name);
  if (ze_collector == nullptr) {
    std::cerr <<
      "[WARNING] Unable to create kernel collector for L0 backend" <<
      std::endl;
  }
  tracer->ze_collector_ = ze_collector;
  
  return tracer;
}

UniTracer::~UniTracer
(
  void
) 
{
  if (ze_collector_ != nullptr) {
    ze_collector_->DisableTracing();
    delete ze_collector_;
  }
}

UniTracer::UniTracer() {}
