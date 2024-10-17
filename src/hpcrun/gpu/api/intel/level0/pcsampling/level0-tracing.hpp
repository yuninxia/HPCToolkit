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


//*****************************************************************************
// global variables
//*****************************************************************************

extern zel_tracer_handle_t tracer_;


//******************************************************************************
// interface operations
//******************************************************************************

#ifdef __cplusplus
extern "C" {
#endif

void
zeroEnableTracing
(
  zel_tracer_handle_t tracer
);

#ifdef __cplusplus
}
#endif

void
zeroDisableTracing
(
  void
);

zel_tracer_handle_t
zeroCreateTracer
(
  ZeCollector* collector
);

void
zeroDestroyTracer
(
  zel_tracer_handle_t tracer
);


#endif // LEVEL0_TRACING_H_
