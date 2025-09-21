// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: BSD-3-Clause

// -*-Mode: C++;-*- // technically C99

//*****************************************************************************
// system includes
//*****************************************************************************

#include <string.h>
#include <pthread.h>
#include <stdio.h>

//*****************************************************************************
// local includes
//*****************************************************************************

#include "level0-kernel-size-map.h"
#include "../../../../memory/hpcrun-malloc.h"
#include "../../../../messages/messages.h"

//*****************************************************************************
// local data structures
//*****************************************************************************

typedef struct kernel_size_entry_s {
  char* kernel_name;
  size_t kernel_size;
  struct kernel_size_entry_s* next;
} kernel_size_entry_t;

//*****************************************************************************
// local variables
//*****************************************************************************

static kernel_size_entry_t* kernel_size_map = NULL;
static pthread_mutex_t kernel_size_map_mutex = PTHREAD_MUTEX_INITIALIZER;

//*****************************************************************************
// private operations
//*****************************************************************************

static kernel_size_entry_t*
kernel_size_entry_new(const char* name, size_t size)
{
  kernel_size_entry_t* entry = hpcrun_malloc(sizeof(kernel_size_entry_t));
  entry->kernel_name = hpcrun_malloc(strlen(name) + 1);
  strcpy(entry->kernel_name, name);
  entry->kernel_size = size;
  entry->next = NULL;
  return entry;
}

static kernel_size_entry_t*
kernel_size_map_find(const char* kernel_name)
{
  kernel_size_entry_t* curr = kernel_size_map;
  while (curr) {
    if (strcmp(curr->kernel_name, kernel_name) == 0) {
      return curr;
    }
    curr = curr->next;
  }
  return NULL;
}

//*****************************************************************************
// interface operations
//*****************************************************************************

void
level0_kernel_size_map_init(void)
{
  // Initialize if needed - currently just uses static initialization
}

void
level0_kernel_size_map_fill_from_symbols(SymbolVector* symbols)
{
  if (!symbols || symbols->nsymbols <= 0) {
    return;
  }

  pthread_mutex_lock(&kernel_size_map_mutex);

  for (int i = 0; i < symbols->nsymbols; i++) {
    if (symbols->symbolName[i] != NULL) {
      // Check if this kernel is already in the map
      kernel_size_entry_t* existing = kernel_size_map_find(symbols->symbolName[i]);
      if (existing) {
        // Update size if different
        if (existing->kernel_size != symbols->symbolSize[i]) {
          TMSG(LEVEL0, "Updating kernel size for %s: %zu -> %zu",
               symbols->symbolName[i], existing->kernel_size, symbols->symbolSize[i]);
          existing->kernel_size = symbols->symbolSize[i];
        }
      } else {
        // Add new entry
        kernel_size_entry_t* new_entry = kernel_size_entry_new(
            symbols->symbolName[i], symbols->symbolSize[i]);
        new_entry->next = kernel_size_map;
        kernel_size_map = new_entry;
        // Debug logging disabled
        TMSG(LEVEL0, "Added kernel to size map: %s (size=%zu)",
             symbols->symbolName[i], symbols->symbolSize[i]);
      }
    }
  }

  pthread_mutex_unlock(&kernel_size_map_mutex);
}

size_t
level0_kernel_size_map_lookup(const char* kernel_name)
{
  if (!kernel_name) {
    return (size_t)-1;
  }

  // Remove trailing null if present
  size_t name_len = strlen(kernel_name);
  char clean_name[256];
  if (name_len > 0 && name_len < sizeof(clean_name)) {
    strcpy(clean_name, kernel_name);
    if (clean_name[name_len - 1] == '\0' && name_len > 1) {
      clean_name[name_len - 1] = '\0';
    }
  } else {
    return (size_t)-1;
  }

  pthread_mutex_lock(&kernel_size_map_mutex);

  kernel_size_entry_t* entry = kernel_size_map_find(clean_name);
  size_t size = entry ? entry->kernel_size : (size_t)-1;

  pthread_mutex_unlock(&kernel_size_map_mutex);

  if (size == (size_t)-1) {
    TMSG(LEVEL0, "Kernel size not found for: %s", clean_name);
  }

  return size;
}