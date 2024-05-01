//==============================================================
// Copyright (C) Intel Corporation
//
// SPDX-License-Identifier: MIT
// =============================================================

#include <iostream>

#include "tracer.h"
 
static UniTracer* tracer = nullptr;

#if 0

void __attribute__((constructor)) Init(void) {
  std::string value = utils::GetEnv("PTI_ENABLE");
  if (value != "1") {
    return;
  }

  if (!tracer) {
    tracer = UniTracer::Create();
  }
}

void __attribute__((destructor)) Fini(void) {
  std::string value = utils::GetEnv("PTI_ENABLE");
  if (value != "1") {
    return;
  }

  if (tracer != nullptr) {
    delete tracer;
  }
}

#endif