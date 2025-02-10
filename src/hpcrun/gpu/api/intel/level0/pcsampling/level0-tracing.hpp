// SPDX-FileCopyrightText: 2002-2024 Rice University
// SPDX-FileCopyrightText: 2024 Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: BSD-3-Clause

// -*-Mode: C++;-*-

#ifndef LEVEL0_TRACING_H_
#define LEVEL0_TRACING_H_

//*****************************************************************************
// level zero includes
//*****************************************************************************

#include <level_zero/layers/zel_tracing_api.h>


//******************************************************************************
// local includes
//******************************************************************************

#include "level0-assert.hpp"
#include "level0-tracing-callbacks.hpp"


//*****************************************************************************
 // Forward Declaration
//*****************************************************************************

class ZeCollector;


//******************************************************************************
// interface operations
//******************************************************************************

bool
level0CreateTracer
(
  ZeCollector* collector,
  const struct hpcrun_foil_appdispatch_level0* dispatch
);

void
level0DestroyTracer
(
  const struct hpcrun_foil_appdispatch_level0* dispatch
);


#endif // LEVEL0_TRACING_H_
