// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: BSD-3-Clause

// -*-Mode: C++;-*- // technically C99

//*****************************************************************************
// system includes
//*****************************************************************************

#define _GNU_SOURCE

#include <string.h>



//*****************************************************************************
// local includes
//*****************************************************************************

#include "../../../../common/lean/splay-uint64.h"
#include "../../../../common/lean/spinlock.h"
#include "../../activity/gpu-activity-channel.h"
#include "../../gpu-splay-allocator.h"
#include "../../activity/gpu-op-placeholders.h"

#include "opencl-context-map.h"


//*****************************************************************************
// macros
//*****************************************************************************

#define DEBUG 0

#include "../../common/gpu-print.h"


#define st_insert                               \
  typed_splay_insert(context)

#define st_lookup                               \
  typed_splay_lookup(context)

#define st_delete                               \
  typed_splay_delete(context)

#define st_forall                               \
  typed_splay_forall(context)

#define st_count                                \
  typed_splay_count(context)

#define st_alloc(free_list)                     \
  typed_splay_alloc(free_list, opencl_context_map_entry_t)

#define st_free(free_list, node)                \
  typed_splay_free(free_list, node)



//*****************************************************************************
// type declarations
//*****************************************************************************

#undef typed_splay_node
#define typed_splay_node(context) opencl_context_map_entry_t

typedef struct typed_splay_node(context) {
  struct typed_splay_node(context) *left;
  struct typed_splay_node(context) *right;
  uint64_t context; // key

  uint32_t context_id;
} typed_splay_node(context);


//******************************************************************************
// local data
//******************************************************************************

static opencl_context_map_entry_t *map_root = NULL;

static opencl_context_map_entry_t *free_list = NULL;

static spinlock_t opencl_context_map_lock = SPINLOCK_UNLOCKED;

static uint32_t cl_context_id = 0;

//*****************************************************************************
// private operations
//*****************************************************************************

typed_splay_impl(context)


static opencl_context_map_entry_t *
opencl_cl_context_map_entry_alloc()
{
  return st_alloc(&free_list);
}


static opencl_context_map_entry_t *
opencl_cl_context_map_entry_new
(
 uint64_t context,
 uint32_t context_id
)
{
  opencl_context_map_entry_t *e = opencl_cl_context_map_entry_alloc();

  e->context = context;
  e->context_id = context_id;

  return e;
}


//*****************************************************************************
// interface operations
//*****************************************************************************

opencl_context_map_entry_t *
opencl_cl_context_map_lookup
(
 uint64_t context
)
{
  spinlock_lock(&opencl_context_map_lock);

  uint64_t id = context;
  opencl_context_map_entry_t *result = st_lookup(&map_root, id);

  spinlock_unlock(&opencl_context_map_lock);

  return result;
}


uint32_t
opencl_cl_context_map_update
(
 uint64_t context
)
{
  spinlock_lock(&opencl_context_map_lock);

  uint32_t ret_context_id = 0;

  opencl_context_map_entry_t *entry = st_lookup(&map_root, context);
  if (entry) {
    entry->context = context;
    entry->context_id = cl_context_id;
  } else {
    opencl_context_map_entry_t *entry =
      opencl_cl_context_map_entry_new(context, cl_context_id);

    st_insert(&map_root, entry);
  }

  // Update cl_context_id
  ret_context_id = cl_context_id++;

  spinlock_unlock(&opencl_context_map_lock);

  return ret_context_id;
}


void
opencl_cl_context_map_delete
(
 uint64_t context
)
{
  spinlock_lock(&opencl_context_map_lock);

  opencl_context_map_entry_t *node = st_delete(&map_root, context);
  st_free(&free_list, node);

  spinlock_unlock(&opencl_context_map_lock);
}


uint32_t
opencl_cl_context_map_entry_context_id_get
(
 opencl_context_map_entry_t *entry
)
{
  return entry->context_id;
}



//*****************************************************************************
// debugging code
//*****************************************************************************

uint64_t
opencl_cl_context_map_count
(
 void
)
{
  return st_count(map_root);
}
