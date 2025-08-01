// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: Apache-2.0

#ifndef FREELIST_ENTRY_DATA_H
#define FREELIST_ENTRY_DATA_H

#define FREELIST_ENTRY_DATA(ENTRY_TYPE)     \
struct {                                    \
  ENTRY_TYPE *next;                         \
}

#endif
