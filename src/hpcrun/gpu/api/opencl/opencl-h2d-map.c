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

#include "opencl-h2d-map.h"



//*****************************************************************************
// macros
//*****************************************************************************

#define DEBUG 0

#include "../../common/gpu-print.h"


#define st_insert                               \
  typed_splay_insert(correlation_id)

#define st_lookup                               \
  typed_splay_lookup(correlation_id)

#define st_delete                               \
  typed_splay_delete(correlation_id)

#define st_forall                               \
  typed_splay_forall(correlation_id)

#define st_count                                \
  typed_splay_count(correlation_id)

#define st_alloc(free_list)                     \
  typed_splay_alloc(free_list, opencl_h2d_map_entry_t)

#define st_free(free_list, node)                \
  typed_splay_free(free_list, node)



//*****************************************************************************
// type declarations
//*****************************************************************************

#undef typed_splay_node
#define typed_splay_node(correlation_id) opencl_h2d_map_entry_t

typedef struct typed_splay_node(correlation_id) {
  struct typed_splay_node(correlation_id) *left;
  struct typed_splay_node(correlation_id) *right;
  uint64_t buffer_id; // key

  uint64_t corr_id;
  size_t size;
  opencl_object_t *cb_info;
} typed_splay_node(correlation_id);


//******************************************************************************
// local data
//******************************************************************************

static opencl_h2d_map_entry_t *map_root = NULL;

static opencl_h2d_map_entry_t *free_list = NULL;

static spinlock_t opencl_h2d_map_lock = SPINLOCK_UNLOCKED;

//*****************************************************************************
// private operations
//*****************************************************************************

typed_splay_impl(correlation_id)


static opencl_h2d_map_entry_t *
opencl_h2d_map_entry_alloc()
{
  return st_alloc(&free_list);
}


static opencl_h2d_map_entry_t *
opencl_h2d_map_entry_new
(
 uint64_t buffer_id,
 uint64_t correlation_id,
 size_t size,
 opencl_object_t *cb_info
)
{
  opencl_h2d_map_entry_t *e = opencl_h2d_map_entry_alloc();

  e->buffer_id = buffer_id;
  e->corr_id = correlation_id;
  e->size = size;
  e->cb_info = cb_info;

  return e;
}


//*****************************************************************************
// interface operations
//*****************************************************************************

opencl_h2d_map_entry_t *
opencl_h2d_map_lookup
(
 uint64_t buffer_id
)
{
  spinlock_lock(&opencl_h2d_map_lock);

  uint64_t id = buffer_id;
  opencl_h2d_map_entry_t *result = st_lookup(&map_root, id);

  spinlock_unlock(&opencl_h2d_map_lock);

  return result;
}


void
opencl_h2d_map_insert
(
 uint64_t buffer_id,
 uint64_t correlation_id,
 size_t size,
 opencl_object_t *cb_info
)
{
  spinlock_lock(&opencl_h2d_map_lock);

  opencl_h2d_map_entry_t *entry = st_lookup(&map_root, buffer_id);
  if (entry) {
    entry->corr_id = correlation_id;
    entry->size = size;
    entry->cb_info = cb_info;
  } else {
    opencl_h2d_map_entry_t *entry =
      opencl_h2d_map_entry_new(buffer_id, correlation_id, size, cb_info);

    st_insert(&map_root, entry);
  }

  spinlock_unlock(&opencl_h2d_map_lock);
}


void
opencl_h2d_map_delete
(
 uint64_t buffer_id
)
{
  spinlock_lock(&opencl_h2d_map_lock);

  opencl_h2d_map_entry_t *node = st_delete(&map_root, buffer_id);
  st_free(&free_list, node);

  spinlock_unlock(&opencl_h2d_map_lock);
}


uint64_t
opencl_h2d_map_entry_buffer_id_get
(
 opencl_h2d_map_entry_t *entry
)
{
  return entry->buffer_id;
}


uint64_t
opencl_h2d_map_entry_correlation_get
(
 opencl_h2d_map_entry_t *entry
)
{
  return entry->corr_id;
}


size_t
opencl_h2d_map_entry_size_get
(
 opencl_h2d_map_entry_t *entry
)
{
  return entry->size;
}


opencl_object_t *
opencl_h2d_map_entry_callback_info_get
(
 opencl_h2d_map_entry_t *entry
)
{
  return entry->cb_info;
}


void
opencl_update_ccts_for_h2d_nodes
(
 opencl_splay_fn_t fn
)
{
        st_forall(map_root, splay_inorder, fn, NULL);
}



//*****************************************************************************
// debugging code
//*****************************************************************************

uint64_t
opencl_h2d_map_count
(
 void
)
{
  return st_count(map_root);
}
