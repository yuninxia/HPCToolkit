// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: Apache-2.0

// -*-Mode: C++;-*- // technically C99

#ifndef libc_functions_h
#define libc_functions_h

//***************************************************************************
// interface operations
//***************************************************************************

/// @brief Lookup an environment variable value using lib getenv
/// @param key to look up in the environment.
/// @return value of key in the environment.
char *libc_getenv(const char *);

#endif /* libc_functions_h */
