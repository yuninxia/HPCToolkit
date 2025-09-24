// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: BSD-3-Clause

// -*-Mode: C++;-*-

//*****************************************************************************
// system includes

#include <new>

// local includes
//*****************************************************************************

#include "level0-collector.hpp"
#include "level0-tracing-callbacks.hpp"
#include "level0-pc-api-receiver.hpp"

extern "C" {
#include "../../../../../messages/messages.h"
}


//******************************************************************************
// public methods
//******************************************************************************

ZeCollector*
ZeCollector::Create
(
  const struct hpcrun_foil_appdispatch_level0* dispatch
)
{
  // Verify that the driver version meets minimum requirements
  if (!level0CheckDriverVersion(1, 2, /*printVersion=*/false, dispatch)) {
    EEMSG("Level0: Failed to create collector: driver version requirements not met");
    return nullptr;
  }

  void* raw = pcsampling::allocMemory(sizeof(ZeCollector));
  if (!raw) {
    EEMSG("Level0: Failed to allocate memory for ZeCollector");
    return nullptr;
  }

  ZeCollector* collector = new (raw) ZeCollector(dispatch);

  if (!level0CreateTracer(collector, dispatch)) {
    EEMSG("Level0: Failed to create tracer for collector");
    ZeCollector::Destroy(collector);
    return nullptr;
  }

  level0InitializeKernelBaseAddressFunction(dispatch);

  return collector;
}

ZeCollector::ZeCollector
(
  const struct hpcrun_foil_appdispatch_level0* dispatch
) : dispatch_(dispatch)
{
  // Enumerate and set up all available devices
  level0EnumerateAndSetupDevices(dispatch);
}

ZeCollector::~ZeCollector()
{  
  // Clean up tracer resources
  level0DestroyTracer(dispatch_);
  
  // Dump collected kernel profiles
  level0DumpKernelProfiles();
}

void
ZeCollector::Destroy
(
  ZeCollector* collector
)
{
  if (!collector) return;
  collector->~ZeCollector();
  pcsampling::freeMemory(collector);
}
