// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: BSD-3-Clause

// -*-Mode: C++;-*- // technically C99

#ifndef cpuset_hwthreads_h
#define cpuset_hwthreads_h

#if defined(__cplusplus)
extern "C" {
#endif

//******************************************************************************
// public operations
//******************************************************************************

//------------------------------------------------------------------------------
//   Function cpuset_hwthreads
//   Purpose:
//     return the number of hardware threads available to this process
//     return 1 if no other value can be computed
//------------------------------------------------------------------------------
unsigned int
cpuset_hwthreads
(
  void
);


#if defined(__cplusplus)
} /* extern "C" */
#endif

#endif
