// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: Apache-2.0

// -*-Mode: C++;-*- // technically C99

#ifndef roctracer_api_h
#define roctracer_api_h



//******************************************************************************
// interface operations
//******************************************************************************

void
roctracer_init
(
 void
);


void
roctracer_flush
(
 void *args,
 int how
);


void
roctracer_fini
(
 void *args,
 int how
);

void
roctracer_enable_counter_collection
(
  void
);

#endif
