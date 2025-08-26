// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: Apache-2.0

// -*-Mode: C++;-*- // technically C99

#ifndef __OMPT_THREAD_H__
#define __OMPT_THREAD_H__


//******************************************************************************
// local includes
//******************************************************************************

#include "ompt-types.h"



//******************************************************************************
// macros
//******************************************************************************

#define MAX_NESTING_LEVELS 128



//******************************************************************************
// interface operations
//******************************************************************************

void
ompt_thread_type_set
(
  ompt_thread_t ttype
);


ompt_thread_t
ompt_thread_type_get
(
  void
);


_Bool
ompt_thread_computes
(
 void
);


region_stack_el_t *
top_region_stack
(
 void
);


region_stack_el_t *
pop_region_stack
(
 void
);


void push_region_stack
(
 ompt_notification_t* notification,
 bool took_sample,
 bool team_master
);


void
clear_region_stack
(
 void
);


int
is_empty_region_stack
(
 void
);

#endif
