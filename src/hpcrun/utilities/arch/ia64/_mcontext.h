// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: Apache-2.0

#ifndef _MCONTEXT_H
#define _MCONTEXT_H

#include <ucontext.h>

static inline void*
ucontext_pc(ucontext_t* context)
{
  return (void*) context->_u._mc.sc_ip;
}

#endif // _MCONTEXT_H
