// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: Apache-2.0

#pragma once
#ifndef HPCRUN_FOIL_CLIENT_PRIVATE_H
#define HPCRUN_FOIL_CLIENT_PRIVATE_H

#ifdef __cplusplus
#error This is a C-only header
#endif

struct hpcrun_foil_hookdispatch_client {
  int (*sampling_is_active)();
  void (*sampling_start)();
  void (*sampling_stop)();
};

#endif // HPCRUN_FOIL_CLIENT_PRIVATE_H
