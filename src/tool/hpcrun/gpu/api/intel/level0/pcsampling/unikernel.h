//==============================================================
// Copyright (C) Intel Corporation
//
// SPDX-License-Identifier: MIT
// =============================================================

#ifndef PTI_TOOLS_UNITRACE_UNIKERNEL_H
#define PTI_TOOLS_UNITRACE_UNIKERNEL_H

#include <iostream>

class UniKernelId{
  public:
    static uint64_t GetKernelId(void) {
        return kernel_id_.fetch_add(1, std::memory_order::memory_order_relaxed);
    }

  private:
    inline static std::atomic<uint64_t> kernel_id_ = 1;	//start with 1
};

#endif // PTI_TOOLS_UNITRACE_UNIKERNEL_H
