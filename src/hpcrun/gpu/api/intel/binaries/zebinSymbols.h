// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: Apache-2.0

//***************************************************************************
//
// File: zebin-symbols.h
//
// Purpose:
//   interface to determine cubin symbol relocation values that will be used
//   by hpcstruct
//
//***************************************************************************

#ifndef zebinSymbols_h
#define zebinSymbols_h

//******************************************************************************
// local includes
//******************************************************************************

#include "symbolVector.h"



//******************************************************************************
// interface functions
//******************************************************************************

SymbolVector *
collectZebinSymbols
(
 const char *zebin_ptr,
 size_t zebin_len
);



#endif
