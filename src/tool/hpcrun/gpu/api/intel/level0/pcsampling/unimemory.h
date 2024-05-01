//============================================================== 
// Copyright (C) Intel Corporation
//
// SPDX-License-Identifier: MIT
// ============================================================= 
#ifndef PTI_TOOLS_UNITRACE_UNIMEMORY_H
#define PTI_TOOLS_UNITRACE_UNIMEMORY_H

namespace UniMemory {
    void AbortIfOutOfMemory(void *ptr);
    void ExitIfOutOfMemory(void *ptr);
}

#endif // PTI_TOOLS_UNITRACE_UNIMEMORY_H