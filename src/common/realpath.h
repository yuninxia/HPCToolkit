// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: Apache-2.0

// -*-Mode: C++;-*- // technically C99

#ifndef realpath_h
#define realpath_h

/*************************** System Include Files ***************************/

/**************************** User Include Files ****************************/

/*************************** Forward Declarations ***************************/

/****************************************************************************/

#if defined(__cplusplus)
extern "C" {
#endif

  /*
   * 'RealPath': returns the absolute path form of 'nm'.
   */
  extern const char* RealPath(const char* nm);

#if defined(__cplusplus)
} /* extern "C" */
#endif

#endif
