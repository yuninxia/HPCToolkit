// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: Apache-2.0

// -*-Mode: C++;-*- // technically C99

#ifndef __OMPT_CALLBACK_H__
#define __OMPT_CALLBACK_H__


//*****************************************************************************
//  macros
//*****************************************************************************

#define ompt_event_may_occur(r) \
  ((r == ompt_set_sometimes) | \
   (r == ompt_set_sometimes_paired) | \
   (r == ompt_set_always))

#endif
