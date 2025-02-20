// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: BSD-3-Clause

// -*-Mode: C++;-*- // technically C99

//***************************************************************************
//
// File:
//   $HeadURL$
//
// Purpose:
//   compiler warnings
//
// Description:
//   [The set of functions, macros, etc. defined in the file]
//
// Author:
//   Nathan Tallent, Rice University.
//
//***************************************************************************

#ifndef include_gcc_attr_h
#define include_gcc_attr_h

//****************************************************************************
//
//****************************************************************************

#ifdef __GNUC__
#  define HPC_GCC_VERSION (__GNUC__ * 1000        \
                           + __GNUC_MINOR__ * 100 \
                           + __GNUC_PATCHLEVEL__)
#  define GCC_ATTR_NORETURN __attribute__ ((noreturn))
#  define GCC_ATTR_UNUSED   __attribute__ ((unused))
#else
#  define HPC_GCC_VERSION 0
#  define GCC_ATTR_NORETURN
#  define GCC_ATTR_UNUSED
#endif


/****************************************************************************/

#endif /* include_gcc_attr_h */
