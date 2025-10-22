// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: Apache-2.0

#include "pcsampling/level0-correlation-device-map.h"

#include <pthread.h>
#include <stdlib.h>

typedef struct device_entry {
  struct device_entry* next;
  ze_device_handle_t device;
  int32_t device_id;
} device_entry_t;

typedef struct cmdlist_entry {
  struct cmdlist_entry* next;
  ze_command_list_handle_t command_list;
  ze_device_handle_t device;
  int32_t device_id;
} cmdlist_entry_t;

static device_entry_t* device_map = NULL;
static pthread_mutex_t device_mutex = PTHREAD_MUTEX_INITIALIZER;

static cmdlist_entry_t* cmdlist_map = NULL;
static pthread_mutex_t cmdlist_mutex = PTHREAD_MUTEX_INITIALIZER;

static device_entry_t*
find_device_entry(ze_device_handle_t device)
{
  device_entry_t* current = device_map;
  while (current != NULL) {
    if (current->device == device) return current;
    current = current->next;
  }
  return NULL;
}

static cmdlist_entry_t*
find_cmdlist_entry(ze_command_list_handle_t command_list)
{
  cmdlist_entry_t* current = cmdlist_map;
  while (current != NULL) {
    if (current->command_list == command_list) return current;
    current = current->next;
  }
  return NULL;
}

void
hpcrun_level0_device_register
(
  ze_device_handle_t device,
  int32_t device_id
)
{
  pthread_mutex_lock(&device_mutex);

  device_entry_t* entry = find_device_entry(device);
  if (entry == NULL) {
    entry = (device_entry_t*)malloc(sizeof(device_entry_t));
    if (entry != NULL) {
      entry->device = device;
      entry->next = device_map;
      device_map = entry;
    }
  }
  if (entry != NULL) {
    if (device_id >= 0 || entry->device_id < 0) {
      entry->device_id = device_id;
    }
  }

  pthread_mutex_lock(&cmdlist_mutex);
  cmdlist_entry_t* cmd = cmdlist_map;
  while (cmd != NULL) {
    if (cmd->device == device) {
      if (device_id >= 0 || cmd->device_id < 0) {
        cmd->device_id = device_id;
      }
    }
    cmd = cmd->next;
  }
  pthread_mutex_unlock(&cmdlist_mutex);

  pthread_mutex_unlock(&device_mutex);
}

void
hpcrun_level0_device_unregister
(
  ze_device_handle_t device
)
{
  pthread_mutex_lock(&device_mutex);

  device_entry_t** prev_next = &device_map;
  device_entry_t* current = device_map;
  while (current != NULL) {
    if (current->device == device) {
      *prev_next = current->next;
      free(current);
      break;
    }
    prev_next = &current->next;
    current = current->next;
  }

  pthread_mutex_unlock(&device_mutex);

  pthread_mutex_lock(&cmdlist_mutex);
  cmdlist_entry_t* cmd = cmdlist_map;
  while (cmd != NULL) {
    if (cmd->device == device) {
      cmd->device_id = -1;
    }
    cmd = cmd->next;
  }
  pthread_mutex_unlock(&cmdlist_mutex);

}

int32_t
hpcrun_level0_device_lookup
(
  ze_device_handle_t device
)
{
  pthread_mutex_lock(&device_mutex);
  device_entry_t* entry = find_device_entry(device);
  int32_t device_id = (entry != NULL) ? entry->device_id : -1;
  pthread_mutex_unlock(&device_mutex);
  return device_id;
}

void
hpcrun_level0_cmdlist_device_register
(
  ze_command_list_handle_t command_list,
  int32_t device_id
)
{
  pthread_mutex_lock(&cmdlist_mutex);

  cmdlist_entry_t* entry = find_cmdlist_entry(command_list);
  if (entry == NULL) {
    entry = (cmdlist_entry_t*)malloc(sizeof(cmdlist_entry_t));
    if (entry != NULL) {
      entry->command_list = command_list;
      entry->device = NULL;
      entry->next = cmdlist_map;
      cmdlist_map = entry;
    }
  }
  if (entry != NULL) {
    if (device_id >= 0 || entry->device_id < 0) {
      entry->device_id = device_id;
    }
  }

  pthread_mutex_unlock(&cmdlist_mutex);

}

void
hpcrun_level0_cmdlist_device_register_with_device
(
  ze_command_list_handle_t command_list,
  ze_device_handle_t device
)
{
  pthread_mutex_lock(&cmdlist_mutex);

  cmdlist_entry_t* entry = find_cmdlist_entry(command_list);
  if (entry == NULL) {
    entry = (cmdlist_entry_t*)malloc(sizeof(cmdlist_entry_t));
    if (entry != NULL) {
      entry->command_list = command_list;
      entry->next = cmdlist_map;
      cmdlist_map = entry;
    }
  }
  if (entry != NULL) {
    entry->device = device;
    int32_t device_id = hpcrun_level0_device_lookup(device);
    if (device_id >= 0 || entry->device_id < 0) {
      entry->device_id = device_id;
    }
  }

  pthread_mutex_unlock(&cmdlist_mutex);

}

void
hpcrun_level0_cmdlist_device_unregister
(
  ze_command_list_handle_t command_list
)
{
  pthread_mutex_lock(&cmdlist_mutex);

  cmdlist_entry_t** prev_next = &cmdlist_map;
  cmdlist_entry_t* current = cmdlist_map;
  while (current != NULL) {
    if (current->command_list == command_list) {
      *prev_next = current->next;
      free(current);
      break;
    }
    prev_next = &current->next;
    current = current->next;
  }

  pthread_mutex_unlock(&cmdlist_mutex);

}

int32_t
hpcrun_level0_cmdlist_device_lookup
(
  ze_command_list_handle_t command_list
)
{
  pthread_mutex_lock(&cmdlist_mutex);
  cmdlist_entry_t* entry = find_cmdlist_entry(command_list);
  int32_t device_id = -1;
  if (entry != NULL) {
    device_id = entry->device_id;
    if (device_id < 0 && entry->device != NULL) {
      device_id = hpcrun_level0_device_lookup(entry->device);
      if (device_id >= 0) {
        entry->device_id = device_id;
      }
    }
  }
  pthread_mutex_unlock(&cmdlist_mutex);

  return device_id;
}
