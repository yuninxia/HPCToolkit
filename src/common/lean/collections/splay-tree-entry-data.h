// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: Apache-2.0

#ifndef GENERIC_SPLAY_TREE_ENTRY_DATA_H
#define GENERIC_SPLAY_TREE_ENTRY_DATA_H 1

#define SPLAY_TREE_ENTRY_DATA(ENTRY_TYPE)       \
struct {                                        \
  ENTRY_TYPE *left;                             \
  ENTRY_TYPE *right;                            \
}

#endif
