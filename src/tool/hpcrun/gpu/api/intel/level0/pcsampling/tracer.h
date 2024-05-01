//==============================================================
// Copyright (C) Intel Corporation
//
// SPDX-License-Identifier: MIT
// =============================================================

#ifndef PTI_TOOLS_UNITRACE_UNIFIED_TRACER_H_
#define PTI_TOOLS_UNITRACE_UNIFIED_TRACER_H_

#include <chrono>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

#include "ze_collector.h"
#include "unimemory.h"

class UniTracer {
 public:
  static UniTracer* Create() {
#if 0
    ze_result_t status = ZE_RESULT_SUCCESS;
    status = zeInit(ZE_INIT_FLAG_GPU_ONLY);
    PTI_ASSERT(status == ZE_RESULT_SUCCESS);
#endif
    UniTracer* tracer = new UniTracer();
    UniMemory::ExitIfOutOfMemory((void *)tracer);

    ZeCollector* ze_collector = nullptr;
    ze_collector = ZeCollector::Create();
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
