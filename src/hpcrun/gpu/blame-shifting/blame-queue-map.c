// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: BSD-3-Clause

//******************************************************************************
// system includes
//******************************************************************************

#define _GNU_SOURCE

#include <assert.h>



//******************************************************************************
// local includes
//******************************************************************************

#include "blame-queue-map.h"

#include "../gpu-splay-allocator.h"
#include "../../../common/lean/splay-uint64.h"
#include "../../../common/lean/spinlock.h"
#include "../../messages/messages.h"



//******************************************************************************
// type declarations
//******************************************************************************

#define queue_insert \
  typed_splay_insert(queue_node)

#define queue_lookup \
  typed_splay_lookup(queue_node)

#define queue_delete \
  typed_splay_delete(queue_node)

#define queue_forall \
  typed_splay_forall(queue_node)

#define queue_count \
  typed_splay_count(queue_node)

#define queue_alloc(free_list) \
  typed_splay_alloc(free_list, queue_map_entry_t)

#define queue_free(free_list, node) \
  typed_splay_free(free_list, node)

#undef typed_splay_node
#define typed_splay_node(queue_node) queue_map_entry_t

typedef struct typed_splay_node(queue_node) {
  struct typed_splay_node(queue_node) *left;
  struct typed_splay_node(queue_node) *right;
  uint64_t queue_id; // key

  queue_node_t *node;
} typed_splay_node(queue_node);

typed_splay_impl(queue_node);



//******************************************************************************
// local data
//******************************************************************************

static queue_map_entry_t *queue_map_root = NULL;
static queue_map_entry_t *queue_map_free_list = NULL;

static spinlock_t queue_map_lock = SPINLOCK_UNLOCKED;



//******************************************************************************
// private operations
//******************************************************************************

static queue_map_entry_t *
queue_node_alloc()
{
  return queue_alloc(&queue_map_free_list);
}


static queue_map_entry_t *
queue_node_new
(
 uint64_t queue_id,
 queue_node_t *node
)
{
  queue_map_entry_t *e = queue_node_alloc();
  e->queue_id = queue_id;
  e->node = node;
  return e;
}



//******************************************************************************
// interface operations
//******************************************************************************

queue_map_entry_t*
queue_map_lookup
(
 uint64_t queue_id
)
{
  spinlock_lock(&queue_map_lock);
  queue_map_entry_t *result = queue_lookup(&queue_map_root, queue_id);
  spinlock_unlock(&queue_map_lock);
  return result;
}


void
queue_map_insert
(
 uint64_t queue_id,
 queue_node_t *node
)
{
  spinlock_lock(&queue_map_lock);
  if (queue_lookup(&queue_map_root, queue_id)) {
    spinlock_unlock(&queue_map_lock);
    assert(false && "entry for a given key should be inserted only once");
    hpcrun_terminate();
  }
  queue_map_entry_t *entry = queue_node_new(queue_id, node);
  queue_insert(&queue_map_root, entry);
  spinlock_unlock(&queue_map_lock);
}


void
queue_map_delete
(
 uint64_t queue_id
)
{
  spinlock_lock(&queue_map_lock);
  queue_map_entry_t *node = queue_delete(&queue_map_root, queue_id);
  queue_free(&queue_map_free_list, node);
  spinlock_unlock(&queue_map_lock);
}


queue_node_t*
queue_map_entry_queue_node_get
(
 queue_map_entry_t *entry
)
{
  return entry->node;
}
