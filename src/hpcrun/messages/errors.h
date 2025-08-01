// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: Apache-2.0

// -*-Mode: C++;-*- // technically C99

#ifndef HPCRUN_MESSAGES_ERRORS_H
#define HPCRUN_MESSAGES_ERRORS_H

#ifndef __cplusplus
#include <stdnoreturn.h>
#endif

/// Abort the process. Does not log any messages.
#ifdef __cplusplus
[[noreturn]]
#else
noreturn
#endif
void hpcrun_terminate();

#endif // HPCRUN_MESSAGES_ERRORS_H
