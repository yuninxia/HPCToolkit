// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: Apache-2.0

//******************************************************************************
// hpctoolkit includes
//******************************************************************************

#include  "../../../../common/lean/collections/stack-entry-data.h"
#include "../../../decl-init-cast.h"

#include "rocm-bufferset.h"



//******************************************************************************
// type declarations
//******************************************************************************

#define TYPE rocprofiler_buffer_id_t

typedef struct rocm_buffer_stack_item_t {
  STACK_ENTRY_DATA(struct rocm_buffer_stack_item_t);
  TYPE buffer_id;
} rocm_buffer_stack_item_t;


// Instantiate bufferset as stack of buffers
#define STACK_DECLARE
#define STACK_DEFINE_INPLACE
#define STACK_PREFIX          rocm_buffer_stack
#define STACK_ENTRY_TYPE      rocm_buffer_stack_item_t
#include "../../../../common/lean/collections/stack.h"



//******************************************************************************
// private variables
//******************************************************************************

static rocm_buffer_stack_t rocm_buffer_stack;



//******************************************************************************
// private operations
//******************************************************************************

static void
rocm_buffer_stack_apply
(
  rocm_buffer_stack_item_t *item,
  void *buffer_op
)
{
  DECL_INIT_CAST(rocm_buffer_op_t, rocm_buffer_op, buffer_op);
  rocm_buffer_op(item->buffer_id);
}


void
rocm_buffer_stack_free
(
  rocm_buffer_stack_t *stack
)
{
  rocm_buffer_stack_item_t *item;
  while ((item = rocm_buffer_stack_pop(stack))) {
    free(item);
  }
}


static rocm_buffer_stack_item_t *
rocm_buffer_stack_item_new
(
  rocprofiler_buffer_id_t buffer_id
)
{
  DECL_INIT_CAST(rocm_buffer_stack_item_t *, item,
    malloc(sizeof(rocm_buffer_stack_item_t)));

  rocm_buffer_stack_init_entry(item);
  item->buffer_id = buffer_id;

  return item;
}



//******************************************************************************
// public interfaces
//******************************************************************************

void
rocm_bufferset_init
(
  void
)
{
  // invariant:
  // either (a) the stack is empty because it was initialized empty, or (b) the
  // stack is non-empty at init time because the process was forked with data
  // on the stack. so if the stack is non-empty, empty it.
  if (!rocm_buffer_stack_empty(&rocm_buffer_stack))
    rocm_buffer_stack_free(&rocm_buffer_stack);
}


void
rocm_bufferset_insert
(
  rocprofiler_buffer_id_t buffer_id
)
{
  rocm_buffer_stack_push(&rocm_buffer_stack, rocm_buffer_stack_item_new(buffer_id));
}


void
rocm_bufferset_apply
(
  rocm_buffer_op_t buffer_op
)
{
  rocm_buffer_stack_for_each_entry
    (&rocm_buffer_stack, rocm_buffer_stack_apply, buffer_op);
}
