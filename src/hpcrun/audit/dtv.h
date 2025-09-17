// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: Apache-2.0

// -*-Mode: C++;-*- // technically C99

#ifndef DTV_H
#define DTV_H

#include <stdbool.h>

/// Glibc implements thread local storage using a data structure known as the
/// Dynamic Thread Vector (DTV).
///
/// As an application begins to execute, glibc allocates the DTV using ld.so's
/// rtld_malloc. As execution continues, if one or more dynamic libraries are
/// loaded that collectively need more space for thread local storage than has
/// been initially reserved in the DTV, glibc will dynamically reallocate the
/// DTV in the heap.
///
/// There is a known bug https://sourceware.org/bugzilla/show_bug.cgi?id=32412
///
/// Prior to glibc 2.42, when LD_AUDIT is used, glibc mistakenly passes a pointer
/// to storage allocated with rtld_malloc to realloc. Realloc fails since the
/// pointer it was passed does not refer to heap data.
///
/// This patch mitigates this bug by copying DTV storage into the heap so that if
/// glibc later attempts to reallocate it, reallocation will succeed.
///
/// WARNING: This patch relies on knowledge of glibc internals, specifically the
/// definition of the DTV structure, knowledge of how the DTV is maintained, and
/// where a pointer to it can be found.

//***************************************************************************
// public operations
//***************************************************************************

/// @brief call this in la_version to capture the initial value of glibc's DTV
void dtv_capture_initial(bool verbose);

/// @brief when LA_ACT_CONSISTENT, reallocate glibc's DTV in the heap if needed
/// @param verbose print debug messages if verbose == true
void dtv_finalize(bool verbose);

#endif  // DTV_H
