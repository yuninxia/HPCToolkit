// SPDX-FileCopyrightText: 2002-2024 Rice University
// SPDX-FileCopyrightText: 2024 Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: BSD-3-Clause

// -*-Mode: C++;-*-

#ifndef LEVEL0_PCSAMPLING_H
#define LEVEL0_PCSAMPLING_H

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
  const struct hpcrun_foil_appdispatch_level0* dispatch
);

void 
zeroPCSamplingFini
(
  void
);

#if defined(__cplusplus)
}
#endif

#endif // LEVEL0_PCSAMPLING_H
