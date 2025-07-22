// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: Apache-2.0

// -*-Mode: C++;-*- // technically C99

#ifndef gpu_timestamp_h
#define gpu_timestamp_h

//******************************************************************************
// system includes
//******************************************************************************

#include <stdint.h>



//******************************************************************************
// interface operations
//******************************************************************************

uint64_t
gpu_timestamp_boottime_to_realtime
(
  uint64_t t
);

#endif
