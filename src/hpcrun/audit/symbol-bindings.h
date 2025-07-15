// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: BSD-3-Clause

// -*-Mode: C++;-*- // technically C99

#ifndef AUDIT_SYMBOL_BINDINGS_H
#define AUDIT_SYMBOL_BINDINGS_H

//***************************************************************************
// system includes
//***************************************************************************

#include <stdbool.h>
#include <stdint.h>



//***************************************************************************
// hpctoolkit includes
//***************************************************************************

#include "audit-api.h"



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
