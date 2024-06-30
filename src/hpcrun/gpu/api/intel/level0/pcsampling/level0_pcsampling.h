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
level0_pcsampling_init
(
  void
);

void 
level0_pcsampling_enable
(
  void
);

void 
level0_pcsampling_fini
(
  void
);

#if defined(__cplusplus)
}
#endif

#endif
