// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: BSD-3-Clause

// -*-Mode: C++;-*- // technically C99

#ifndef level0_kernel_size_map_h
#define level0_kernel_size_map_h

//*****************************************************************************
// system includes
//*****************************************************************************

#include <stddef.h>

//*****************************************************************************
// local includes
//*****************************************************************************

#include "../binaries/zebinSymbols.h"

//*****************************************************************************
// interface operations
//*****************************************************************************

#ifdef __cplusplus
extern "C" {
#endif

// Initialize the kernel size map
void
level0_kernel_size_map_init(void);

// Fill the kernel size map from symbol vector
void
level0_kernel_size_map_fill_from_symbols(SymbolVector* symbols);

// Get the size of a kernel by name
// Returns (size_t)-1 if not found
size_t
level0_kernel_size_map_lookup(const char* kernel_name);

#ifdef __cplusplus
}
#endif

#endif // level0_kernel_size_map_h