// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: Apache-2.0

// -*-Mode: C++;-*- // technically C99

#ifndef AUDIT_SYMBOL_BINDINGS_H
#define AUDIT_SYMBOL_BINDINGS_H

//***************************************************************************
// include files
//***************************************************************************

#include "audit-api.h"

#include <stdbool.h>
#include <stdint.h>



//***************************************************************************
// interface operations
//***************************************************************************

bool
audit_symbol_binding
(
  const char *symname,
  uintptr_t *binding
);


void
libhpcrun_initialization_begin
(
  void
);


void
libhpcrun_initialization_end
(
  void
);

#endif  // AUDIT_SYMBOL_BINDINGS_H
