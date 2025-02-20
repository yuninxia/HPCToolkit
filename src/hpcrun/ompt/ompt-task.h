// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: BSD-3-Clause

// -*-Mode: C++;-*- // technically C99

#ifndef __OMPT_TASK_H__
#define __OMPT_TASK_H__


//*****************************************************************************
// local includes
//*****************************************************************************

#include "omp-tools.h"


//*****************************************************************************
// interface operations
//*****************************************************************************

void
ompt_task_register_callbacks
(
 ompt_set_callback_t ompt_set_callback_fn
);

#endif
