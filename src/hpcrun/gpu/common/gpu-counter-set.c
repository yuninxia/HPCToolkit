// SPDX-FileCopyrightText: 2024 Rice University
// SPDX-FileCopyrightText: 2024 Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: BSD-3-Clause

//******************************************************************************
// system includes
//******************************************************************************

#include <stdio.h>
#include <string.h>



//******************************************************************************
// hpctoolkit includes
//******************************************************************************

#define _GNU_SOURCE

#include "../../../common/lean/collections/splay-tree-entry-data.h"
#include "../../decl-init-cast.h"

#include "gpu-counter-set.h"



//******************************************************************************
// debugging
//******************************************************************************

#define DEBUG 0

#include "gpu-print.h"


//******************************************************************************
// generic code - splay tree
//******************************************************************************

typedef struct gpu_counter_set_entry_t {
  SPLAY_TREE_ENTRY_DATA(struct gpu_counter_set_entry_t);
  const char *counter_name;
} gpu_counter_set_entry_t;


#define SPLAY_TREE_PREFIX         st
#define SPLAY_TREE_KEY_TYPE       const char *
#define SPLAY_TREE_KEY_FIELD      counter_name
#define SPLAY_TREE_ENTRY_TYPE     gpu_counter_set_entry_t

#define SPLAY_TREE_LT(A, B) (strcmp(A, B) < 0)
#define SPLAY_TREE_GT(A, B) (strcmp(A, B) > 0)
#define SPLAY_TREE_EQ(A, B) (strcmp(A, B) == 0)

#define SPLAY_TREE_DEFINE_INPLACE
#include "../../../common/lean/collections/splay-tree.h"



//******************************************************************************
// type declarations
//******************************************************************************

struct gpu_counter_set_t {
  st_t root;
};


typedef struct gpu_counter_set_apply_helper_arg_t {
  gpu_counter_set_apply_fn_t apply_fn;
  void *apply_arg;
} gpu_counter_set_apply_helper_arg_t;


typedef struct gpu_counter_set_difference_state_t {
  gpu_counter_set_t *subset;
  gpu_counter_set_t *difference;
} gpu_counter_set_difference_state_t;



//*****************************************************************************
// private operations
//*****************************************************************************

static void
gpu_counter_set_difference_helper
(
  const char *counter_name,
  void *arg
)
{
  DECL_INIT_CAST(gpu_counter_set_difference_state_t *, state, arg);

  bool found = gpu_counter_set_find(state->subset, counter_name);
  if (!found) {
    gpu_counter_set_insert(state->difference, counter_name);
  }
}


static void
gpu_counter_set_dump_counter
(
  const char *counter_name,
  void *arg
)
{
  fprintf(stderr, "  %s\n" , counter_name);
}


static void
gpu_counter_set_apply_helper
(
  gpu_counter_set_entry_t *entry,
  void *arg
)
{
  DECL_INIT_CAST(gpu_counter_set_apply_helper_arg_t *, helper_args, arg);

  PRINT("gpu_counter_set_apply_helper: call %p(%p, %s, %p)\n",
    helper_args->apply_fn, &entry->counter_name, helper_args->apply_arg);

  helper_args->apply_fn(entry->counter_name, helper_args->apply_arg);
}


static gpu_counter_set_entry_t *
node_new
(
  const char *name
)
{
  DECL_INIT_CAST(gpu_counter_set_entry_t *, entry,
                 malloc(sizeof(gpu_counter_set_entry_t)));
  memset(entry, 0, sizeof(*entry));
  entry->counter_name = strdup(name);
  return entry;
}


static void
node_delete
(
  gpu_counter_set_entry_t * entry
)
{
  free((char *)entry->counter_name);
  free(entry);
}


//*****************************************************************************
// interface operations
//*****************************************************************************

gpu_counter_set_t *
gpu_counter_set_new
(
  void
)
{
  DECL_INIT_CAST(gpu_counter_set_t *, set, malloc(sizeof(gpu_counter_set_t)));

  set->root = (st_t) SPLAY_TREE_INITIALIZER;

  return set;
}


bool
gpu_counter_set_nonempty
(
  gpu_counter_set_t *set
)
{
  return set && !st_empty(&set->root);
}


bool
gpu_counter_set_insert
(
  gpu_counter_set_t *set,
  const char *name
)
{
  if (set == NULL) return false;

  DECL_INIT_CAST(gpu_counter_set_entry_t *, entry,
                 node_new(name));

  bool inserted = st_insert(&set->root, entry);

  if (!inserted) {
    node_delete(entry);
  }

  return inserted;
}


bool
gpu_counter_set_find
(
  gpu_counter_set_t *set,
  const char *counter_name
)
{
  if (set == NULL) return false;

  gpu_counter_set_entry_t *entry =
    st_lookup(&set->root, counter_name);

  return entry != NULL;
}


void
gpu_counter_set_apply
(
  gpu_counter_set_t *set,
  gpu_counter_set_apply_fn_t apply_fn,
  void *apply_arg
)
{
  if (set == NULL) return;

  PRINT("gpu_counter_set_apply(%p, %p\n", apply_fn, apply_arg);

  gpu_counter_set_apply_helper_arg_t helper_args;

  helper_args.apply_fn = apply_fn;
  helper_args.apply_arg = apply_arg;

  st_for_each(&set->root, gpu_counter_set_apply_helper, &helper_args);
}


void
gpu_counter_set_delete
(
  gpu_counter_set_t *set
)
{
  if (set == NULL) return;

  while (!st_empty(&set->root)) {
    gpu_counter_set_entry_t *entry = st_delete_root(&set->root);
    node_delete(entry);
  }
  free(set);
}


gpu_counter_set_t *
gpu_counter_set_difference
(
  gpu_counter_set_t *set1,
  gpu_counter_set_t *set2
)
{
  gpu_counter_set_difference_state_t state;
  state.subset = set2;
  state.difference = gpu_counter_set_new();

  // after this operation, state.difference will point to a set
  // containing members of set1 not in set2
  gpu_counter_set_apply(set1, gpu_counter_set_difference_helper, &state);

  return state.difference;
}


void
gpu_counter_set_dump
(
  gpu_counter_set_t *set,
  const char *name
)
{
  if (name) fprintf(stderr, "%s {\n", name);

  gpu_counter_set_apply(set, gpu_counter_set_dump_counter, 0);

  if (name) fprintf(stderr, "}\n");
}
