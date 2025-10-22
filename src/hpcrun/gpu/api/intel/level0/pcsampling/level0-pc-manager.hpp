// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: Apache-2.0
//
// SPDX-License-Identifier: MIT
// Copyright (C) 2024 Intel Corporation
// This file was inspired by and uses some code fragments from Intel's
// MIT-licensed pti-gpu (https://github.com/intel/pti-gpu)

// -*-Mode: C++;-*-

#ifndef LEVEL0_PC_H
#define LEVEL0_PC_H

//******************************************************************************
// interface operations
//******************************************************************************

#if defined(__cplusplus)
extern "C" {
#endif

/**
 * Initialize PC sampling with the provided Level Zero dispatch interface.
 * This function is called once per process to set up metric collection.
 * Initializes the memory cache and creates the collector and profiler.
 */
void
level0PCSamplingInit
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

/**
 * Check if PC sampling is enabled and successfully initialized.
 * Returns true if PC sampling is ready to use, false otherwise.
 */
bool
level0PCSamplingIsReady
(
  void
);

#if defined(__cplusplus)
}
#endif

#endif // LEVEL0_PC_H
