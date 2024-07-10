// SPDX-FileCopyrightText: 2002-2024 Rice University
// SPDX-FileCopyrightText: 2024 Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: BSD-3-Clause

// -*-Mode: C++;-*- // technically C99

#ifndef level0_pcsampling_h
#define level0_pcsampling_h

//******************************************************************************
// interface operations
//******************************************************************************

#if defined(__cplusplus)
extern "C" {
#endif

void 
zeroPCSamplingInit
(
  void
);

void
zeroPCSamplingEnable
(
  void
);

void 
zeroPCSamplingFini
(
  void
);

#if defined(__cplusplus)
}
#endif

#endif
