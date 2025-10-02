// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: Apache-2.0

// -*-Mode: C++;-*- // technically C99

#ifndef hpcrun_env_h
#define hpcrun_env_h

//***************************************************************************
// system operations
//***************************************************************************

#include <stdbool.h>



//***************************************************************************
// global variables
//***************************************************************************

// Names for option environment variables
extern const char* HPCRUN_OUT_PATH;

extern const char* HPCRUN_TRACE;

extern const char* HPCRUN_EVENT_LIST;
extern const char* HPCRUN_MEMSIZE;
extern const char* HPCRUN_LOW_MEMSIZE;

extern const char* HPCRUN_ABORT_LIBC;



//***************************************************************************
// interface operations
//***************************************************************************

bool hpcrun_get_env_bool(const char *);

bool hpcrun_get_env_int(const char *, int *);

#endif /* hpcrun_env_h */
