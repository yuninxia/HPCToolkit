// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: Apache-2.0

//******************************************************************************
// system includes
//******************************************************************************

#include <stdio.h>
#include <string.h>



//******************************************************************************
// hpctoolkit includes
//******************************************************************************

#define _GNU_SOURCE

#include "../../../../common/lean/collections/splay-tree-entry-data.h"
#include "../../../decl-init-cast.h"

#include "cuda-function-name-map.h"



//******************************************************************************
// debugging
//******************************************************************************

#define DEBUG 0

#include "../../common/gpu-print.h"


//******************************************************************************
// generic code - splay tree
//******************************************************************************

typedef struct cuda_function_name_map_entry_t {
  SPLAY_TREE_ENTRY_DATA(struct cuda_function_name_map_entry_t);
  char *name; // key
  ip_normalized_t pc;
} cuda_function_name_map_entry_t;


#define SPLAY_TREE_PREFIX         st
#define SPLAY_TREE_KEY_TYPE       const char *
#define SPLAY_TREE_KEY_FIELD      name
#define SPLAY_TREE_ENTRY_TYPE     cuda_function_name_map_entry_t

#define SPLAY_TREE_LT(A, B) (strcmp(A, B) < 0)
#define SPLAY_TREE_GT(A, B) (strcmp(A, B) > 0)
#define SPLAY_TREE_EQ(A, B) (strcmp(A, B) == 0)

#define SPLAY_TREE_DEFINE_INPLACE
#include "../../../../common/lean/collections/splay-tree.h"



//******************************************************************************
// local variables
//******************************************************************************

static st_t root;



//*****************************************************************************
// private operations
//*****************************************************************************

static void
cuda_function_name_map_dump_entry
(
  cuda_function_name_map_entry_t *entry,
  void *arg
)
{
  fprintf(stderr, "  %s -> " IP_NORMALIZED_PRINT_FORMAT "\n" , entry->name,
    IP_NORMALIZED_PRINT_ARGS(entry->pc));
}


static cuda_function_name_map_entry_t *
node_new
(
  const char *name,
  ip_normalized_t pc
)
{
  DECL_INIT_CAST(cuda_function_name_map_entry_t *, entry,
                 malloc(sizeof(cuda_function_name_map_entry_t)));
  memset(entry, 0, sizeof(*entry));
  entry->name = strdup(name);
  entry->pc = pc;
  return entry;
}


static void
node_delete
(
  cuda_function_name_map_entry_t *entry
)
{
  free((char *)entry->name);
  free(entry);
}



//*****************************************************************************
// interface operations
//*****************************************************************************

void
cuda_function_name_map_init
(
  void
)
{
  root = (st_t) SPLAY_TREE_INITIALIZER;
  PRINT("cuda_function_name_map_init\n");
}


bool
cuda_function_name_map_insert
(
  const char *name,
  ip_normalized_t pc
)
{
  if (ip_normalized_eq(&pc, &ip_normalized_NULL)) return false;

  DECL_INIT_CAST(cuda_function_name_map_entry_t *, entry, node_new(name, pc));

  bool inserted = st_insert(&root, entry);

  if (!inserted) {
    node_delete(entry);
  }

  PRINT("cuda_function_name_map_insert: %s -> " IP_NORMALIZED_PRINT_FORMAT
    " inserted=%d\n", name, IP_NORMALIZED_PRINT_ARGS(pc), inserted);

  return inserted;
}

ip_normalized_t
cuda_function_name_map_lookup
(
 const char *name
)
{
  if (st_empty(&root)) return ip_normalized_NULL;

  cuda_function_name_map_entry_t *entry = st_lookup(&root, name);

  ip_normalized_t pc = entry ? entry->pc : ip_normalized_NULL;

  PRINT("cuda_function_name_map_find: %s -> %s -> " IP_NORMALIZED_PRINT_FORMAT
    "\n", name, IP_NORMALIZED_PRINT_ARGS(pc));

  return pc;
}


void
cuda_function_name_map_dump
(
  void
)
{
  if (st_empty(&root)) return;

  st_for_each(&root, cuda_function_name_map_dump_entry, 0);
}
