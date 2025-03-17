// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
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

/**
 * Initialize the PC sampling subsystem.
 * Creates necessary directories and prepares for metric collection.
 */
void 
level0PCSamplingInit
(
  void
);

/**
 * Enable PC sampling with the provided Level Zero dispatch interface.
 * This function is called once per process to set up metric collection.
 */
void
level0PCSamplingEnable
(
  const struct hpcrun_foil_appdispatch_level0* dispatch
);

/**
 * Clean up PC sampling resources.
 * Called during program termination to free resources and clean up temporary files.
 */
void 
level0PCSamplingFini
(
  void
);

#if defined(__cplusplus)
}
#endif

#endif // LEVEL0_PCSAMPLING_H
