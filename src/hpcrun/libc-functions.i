// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: BSD-3-Clause

// -*-Mode: C++;-*- // technically C99

#ifndef libc_functions_i
#define libc_functions_i

//***************************************************************************
// hpctoolkit includes
//***************************************************************************

#include "libc-functions.h"
#include "audit/audit-api.h"



//***************************************************************************
// interface operations
//***************************************************************************

char *
libc_getenv
(
  const char *key
)
{
  return auditor_exports()->getenv(key);
}

#endif
