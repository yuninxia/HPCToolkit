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

#include "../../../../../../common/lean/splay-uint64.h"
#include "../../../../../../common/lean/spinlock.h"
#include "../../../../gpu-splay-allocator.h"

#include "queue-context-map.h"



//*****************************************************************************
// macros
//*****************************************************************************

#define DEBUG 0

#define st_insert                               \
  typed_splay_insert(queue)

#define st_lookup                               \
  typed_splay_lookup(queue)

#define st_delete                               \
  typed_splay_delete(queue)

#define st_forall                               \
  typed_splay_forall(queue)

#define st_count                                \
  typed_splay_count(queue)

#define st_alloc(free_list)                     \
  typed_splay_alloc(free_list, queue_context_map_entry_t)

#define st_free(free_list, node)                \
  typed_splay_free(free_list, node)



//*****************************************************************************
// type declarations
//*****************************************************************************

#undef typed_splay_node
#define typed_splay_node(queue) queue_context_map_entry_t

typedef struct typed_splay_node(queue) {
  struct typed_splay_node(queue) *left;
  struct typed_splay_node(queue) *right;
  uint64_t queue_id; // key

  uint64_t context_id;
} typed_splay_node(queue);


//******************************************************************************
// local data
//******************************************************************************

static queue_context_map_entry_t *map_root = NULL;

static queue_context_map_entry_t *free_list = NULL;

static spinlock_t queue_context_map_lock = SPINLOCK_UNLOCKED;



//*****************************************************************************
// private operations
//*****************************************************************************

typed_splay_impl(queue)


static queue_context_map_entry_t *
queue_context_map_entry_alloc()
{
  return st_alloc(&free_list);
}


static queue_context_map_entry_t *
queue_context_map_entry_new
(
 uint64_t queue_id,
 uint64_t context_id
)
{
  queue_context_map_entry_t *e = queue_context_map_entry_alloc();

  e->queue_id = queue_id;
  e->context_id = context_id;

  return e;
}


//*****************************************************************************
// interface operations
//*****************************************************************************

queue_context_map_entry_t *
queue_context_map_lookup
(
 uint64_t queue_id
)
{
  spinlock_lock(&queue_context_map_lock);

  uint64_t id = queue_id;
  queue_context_map_entry_t *result = st_lookup(&map_root, id);

  spinlock_unlock(&queue_context_map_lock);

  return result;
}


void
queue_context_map_insert
(
 uint64_t queue_id,
 uint64_t context_id
)
{
  spinlock_lock(&queue_context_map_lock);

  queue_context_map_entry_t *entry = st_lookup(&map_root, queue_id);
  if (entry) {
    entry->context_id = context_id;
  } else {
    queue_context_map_entry_t *entry =
      queue_context_map_entry_new(queue_id, context_id);

    st_insert(&map_root, entry);
  }

  spinlock_unlock(&queue_context_map_lock);
}


void
queue_context_map_delete
(
 uint64_t queue_id
)
{
  spinlock_lock(&queue_context_map_lock);

  queue_context_map_entry_t *node = st_delete(&map_root, queue_id);
  st_free(&free_list, node);

  spinlock_unlock(&queue_context_map_lock);
}


uint64_t
queue_context_map_entry_queue_id_get
(
 queue_context_map_entry_t *entry
)
{
  return entry->queue_id;
}


uint64_t
queue_context_map_entry_context_id_get
(
 queue_context_map_entry_t *entry
)
{
  return entry->context_id;
}



//*****************************************************************************
// debugging code
//*****************************************************************************

uint64_t
queue_context_map_count
(
 void
)
{
  return st_count(map_root);
}
