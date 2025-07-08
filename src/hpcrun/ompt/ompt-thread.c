// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: Apache-2.0

// -*-Mode: C++;-*- // technically C99

//******************************************************************************
// local includes
//******************************************************************************

#define _GNU_SOURCE

#include "ompt-specific.h"
#include "ompt-thread.h"

#include "../messages/messages.h"



//******************************************************************************
// interface operations
//******************************************************************************

void
ompt_thread_type_set
(
 ompt_thread_t ttype
)
{
  int * ompt_thread_type = OMPT_GETSPECIFIC(ompt_thread_type);
  *ompt_thread_type = ttype;
}


ompt_thread_t
ompt_thread_type_get
(
)
{
  int * ompt_thread_type = OMPT_GETSPECIFIC(ompt_thread_type);
  return *ompt_thread_type;
}


_Bool
ompt_thread_computes
(
 void
)
{
  switch(ompt_thread_type_get()) {
  case ompt_thread_initial:
  case ompt_thread_worker:
    return true;
  case ompt_thread_other:
  case ompt_thread_unknown:
  default:
    break;
  }
  return false;
}



region_stack_el_t*
top_region_stack
(
 void
)
{
  region_stack_el_t * region_stack = OMPT_GETSPECIFIC(region_stack[0]);
  int * top_index = OMPT_GETSPECIFIC(top_index);

  // FIXME: is invalid value for region ID
  return (*top_index > -1) ? &region_stack[*top_index] : NULL;
}

region_stack_el_t*
pop_region_stack
(
 void
)
{
  region_stack_el_t * region_stack = OMPT_GETSPECIFIC(region_stack[0]);
  int * top_index = OMPT_GETSPECIFIC(top_index);

  return (*top_index > -1) ? &region_stack[(*top_index)--] : NULL;
}


void
push_region_stack
(
 ompt_notification_t* notification,
 bool took_sample,
 bool team_master
)
{
  region_stack_el_t * region_stack = OMPT_GETSPECIFIC(region_stack[0]);
  int * top_index = OMPT_GETSPECIFIC(top_index);

  // FIXME: potential place of segfault, when stack is full
  (*top_index)++;
  region_stack[*top_index].notification = notification;
  region_stack[*top_index].took_sample = took_sample;
  region_stack[*top_index].team_master = team_master;
}


void
clear_region_stack
(
 void
)
{
  int * top_index = OMPT_GETSPECIFIC(top_index);
  *top_index = -1;
}


int
is_empty_region_stack
(
 void
)
{
  int * top_index = OMPT_GETSPECIFIC(top_index);
  return *top_index < 0;
}
