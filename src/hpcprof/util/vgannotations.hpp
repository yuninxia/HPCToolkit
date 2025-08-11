// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: Apache-2.0

// -*-Mode: C++;-*-

#ifndef HPCTOOLKIT_PROFILE_UTIL_VGANNOTATIONS_H
#define HPCTOOLKIT_PROFILE_UTIL_VGANNOTATIONS_H

// These two headers need to be loaded in exactly this order, otherwise the macros get
// redefined and all heck breaks loose.
//
// clang-format off
#include "valgrind/helgrind.h"
#include "valgrind/drd.h"
// clang-format on

#define _GLIBCXX_SYNCHRONIZATION_HAPPENS_BEFORE(addr) ANNOTATE_HAPPENS_BEFORE(addr)
#define _GLIBCXX_SYNCHRONIZATION_HAPPENS_AFTER(addr)  ANNOTATE_HAPPENS_AFTER(addr)

#endif  // HPCTOOLKIT_PROFILE_UTIL_VGANNOTATIONS_H
