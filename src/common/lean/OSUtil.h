// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: Apache-2.0

// -*-Mode: C++;-*- // technically C99

//***************************************************************************
//
// File:
//   $HeadURL$
//
// Purpose:
//   OS Utilities
//
// Description:
//   [The set of functions, macros, etc. defined in the file]
//
// Author:
//   Nathan Tallent, John Mellor-Crummey, Rice University.
//
//***************************************************************************

#ifndef support_lean_OSUtil_h
#define support_lean_OSUtil_h

//***************************************************************************
// system include files
//***************************************************************************

#include <stddef.h>
#include <inttypes.h>


//***************************************************************************
// user include files
//***************************************************************************




//***************************************************************************
// macros
//***************************************************************************

#define HOSTID_FORMAT "%08" PRIx32



//***************************************************************************
// forward declarations
//***************************************************************************

typedef char *(*libc_getenv_t)(const char *);



//***************************************************************************
// forward declarations
//***************************************************************************

#ifdef __cplusplus
extern "C" {
#endif

unsigned int
OSUtil_pid();

const char*
OSUtil_local_rank(libc_getenv_t libc_getenv);

long long
OSUtil_rank(libc_getenv_t libc_getenv);

const char*
OSUtil_jobid(libc_getenv_t libc_getenv);

uint32_t
OSUtil_hostid(libc_getenv_t libc_getenv);

// set the buffer into the customized kernel name
// @param buffer: (in/out) the buffer to store the new name
// @param max_chars: the number of maximum characters the buffer can store
// @return the number of characters copied.
int
OSUtil_setCustomKernelName(char *buffer, size_t max_chars, libc_getenv_t libc_getenv);

// similar to above, but with fake name symbol < and >
int
OSUtil_setCustomKernelNameWrap(char *buffer, size_t max_chars, libc_getenv_t libc_getenv);

#ifdef __cplusplus
}
#endif


//***************************************************************************

#endif /* support_lean_OSUtil_h */
