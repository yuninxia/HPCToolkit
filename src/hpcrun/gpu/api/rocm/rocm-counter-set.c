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

#include "../../../../common/lean/collections/splay-tree-entry-data.h"
#include "../../../decl-init-cast.h"

#include "rocm-counter-set.h"



//******************************************************************************
// debugging
//******************************************************************************

#define DEBUG 0

#include "../../../gpu/common/gpu-print.h"



//******************************************************************************
// type declarations
//******************************************************************************

typedef struct {
  rocm_counter_set_apply_fn_t apply_fn;
  void *apply_arg;
} rocm_counter_set_apply_helper_arg_t;


//******************************************************************************
// generic code - splay tree
//******************************************************************************

typedef struct rocm_counter_set_entry_t {
  SPLAY_TREE_ENTRY_DATA(struct rocm_counter_set_entry_t);
  rocprofiler_counter_info_v0_t counter_info;
  unsigned int index;
} rocm_counter_set_entry_t;


#define SPLAY_TREE_PREFIX         st
#define SPLAY_TREE_KEY_TYPE       const char *
#define SPLAY_TREE_KEY_FIELD      counter_info.name
#define SPLAY_TREE_ENTRY_TYPE     rocm_counter_set_entry_t

#define SPLAY_TREE_LT(A, B) (strcmp(A, B) < 0)
#define SPLAY_TREE_GT(A, B) (strcmp(A, B) > 0)
#define SPLAY_TREE_EQ(A, B) (strcmp(A, B) == 0)

#define SPLAY_TREE_DEFINE_INPLACE
#include "../../../../common/lean/collections/splay-tree.h"



//******************************************************************************
// private data
//******************************************************************************

static st_t counter_set = SPLAY_TREE_INITIALIZER;
static unsigned int counter_index = 0;



//*****************************************************************************
// private operations
//*****************************************************************************

static void
rocm_counter_set_dump_counter
(
  rocprofiler_counter_info_v0_t *i,
  unsigned int *index,
  void *arg
)
{
  fprintf(stderr, "%s: constant=%d derived=%d\n", i->name,  i->is_constant, i->is_derived);
  if (i->expression) {
    fprintf(stderr, "16%sExpr=%s\n", "",i->expression);
  }
  if (i->block) {
    fprintf(stderr, "16%sBlock=%s\n", "",i->block);
  }
  if (i->description) {
    fprintf(stderr, "16%sDesc=%s\n\n", "",i->description);
  }
}


static void
rocm_counter_set_apply_helper
(
  rocm_counter_set_entry_t *entry,
  void *arg
)
{
  DECL_INIT_CAST(rocm_counter_set_apply_helper_arg_t *, helper_args, arg);

  PRINT("rocm_counter_set_apply_helper: call %p(%p, %p, %p)\n",
    helper_args->apply_fn, &entry->counter_info, &entry->index, helper_args->apply_arg);

  helper_args->apply_fn(&entry->counter_info, &entry->index,
    helper_args->apply_arg);
}



//*****************************************************************************
// interface operations
//*****************************************************************************

void
rocm_counter_set_init
(
  void
)
{
  counter_set = (st_t) SPLAY_TREE_INITIALIZER;
  counter_index = 0;
}


bool
rocm_counter_set_nonempty
(
  void
)
{
  return !st_empty(&counter_set);
}


bool
rocm_counter_set_insert
(
  rocprofiler_counter_info_v0_t *counter_info
)
{
  DECL_INIT_CAST(rocm_counter_set_entry_t *, entry,
                 malloc(sizeof(rocm_counter_set_entry_t)));

  memset(entry,0,sizeof(rocm_counter_set_entry_t));

  *entry = (rocm_counter_set_entry_t) {
    .counter_info = *counter_info
  };

  bool inserted = st_insert(&counter_set, entry);

  if (!inserted) {
    free(entry);
  } else {
    entry->index = counter_index++;
  }

  return inserted;
}


bool
rocm_counter_set_find
(
  const char *counter_name,
  rocprofiler_counter_info_v0_t **counter_info,    // return value
  unsigned int *index                              // return value
)
{
  rocm_counter_set_entry_t *entry =
    st_lookup(&counter_set, counter_name);

  bool found = entry != NULL;
  if (found) {
    *counter_info = &entry->counter_info;
    *index = entry->index;
  };

  return found;
}


unsigned int
rocm_counter_set_size
(
  void
)
{
  return counter_index; // the next index doubles as the size
}


void
rocm_counter_set_apply
(
  rocm_counter_set_apply_fn_t apply_fn,
  void *apply_arg
)
{
  PRINT("rocm_counter_set_apply(%p, %p\n", apply_fn, apply_arg);

  rocm_counter_set_apply_helper_arg_t helper_args;

  helper_args.apply_fn = apply_fn;
  helper_args.apply_arg = apply_arg;

  st_for_each(&counter_set, rocm_counter_set_apply_helper, &helper_args);
}


void
rocm_counter_set_dump
(
  void
)
{
  fprintf(stderr, "rocm_counter_set_dump: {\n");

  rocm_counter_set_apply(rocm_counter_set_dump_counter, 0);

  fprintf(stderr, "}\n");
}
