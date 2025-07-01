// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: Apache-2.0

#include <stdlib.h>

// This helper constructor calls malloc, which triggers the setup in pthread-key-canary.c
__attribute__((constructor)) static void init() { malloc(0); }

void helper() {}
