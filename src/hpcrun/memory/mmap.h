// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: Apache-2.0

#ifndef MMAP_H
#define MMAP_H

#include <stdlib.h>

void* hpcrun_mmap_anon(size_t size);
void hpcrun_mmap_init(void);

#endif // MMAP_H
