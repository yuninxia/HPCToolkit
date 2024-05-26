//==============================================================
// Copyright (C) Intel Corporation
//
// SPDX-License-Identifier: MIT
// =============================================================

#ifndef PTI_TOOLS_UNITRACE_UNIFIED_TRACER_H_
#define PTI_TOOLS_UNITRACE_UNIFIED_TRACER_H_

#include <iostream>
#include <string>

#include "ze_collector.h"

class UniTracer {
 public:
  static UniTracer* Create(const std::string& data_dir_name) {
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

  ~UniTracer() {
    if (ze_collector_ != nullptr) {
      ze_collector_->DisableTracing();
      delete ze_collector_;
    }
  }

  UniTracer(const UniTracer& that) = delete;
  UniTracer& operator=(const UniTracer& that) = delete;

 private:
  UniTracer(){}

 private:
  ZeCollector* ze_collector_ = nullptr;
};

#endif // PTI_TOOLS_UNITRACE_UNIFIED_TRACER_H_
