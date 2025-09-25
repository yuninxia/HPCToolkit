// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: BSD-3-Clause

#ifndef LEVEL0_CORRELATION_DEVICE_MAP_H
#define LEVEL0_CORRELATION_DEVICE_MAP_H

#include <level_zero/ze_api.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void
hpcrun_level0_device_register
(
  ze_device_handle_t device,
  int32_t device_id
);

void
hpcrun_level0_device_unregister
(
  ze_device_handle_t device
);

int32_t
hpcrun_level0_device_lookup
(
  ze_device_handle_t device
);

void
hpcrun_level0_cmdlist_device_register
(
  ze_command_list_handle_t command_list,
  int32_t device_id
);

void
hpcrun_level0_cmdlist_device_register_with_device
(
  ze_command_list_handle_t command_list,
  ze_device_handle_t device
);

void
hpcrun_level0_cmdlist_device_unregister
(
  ze_command_list_handle_t command_list
);

int32_t
hpcrun_level0_cmdlist_device_lookup
(
  ze_command_list_handle_t command_list
);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // LEVEL0_CORRELATION_DEVICE_MAP_H
