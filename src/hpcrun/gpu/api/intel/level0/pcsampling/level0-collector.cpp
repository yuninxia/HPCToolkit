// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: BSD-3-Clause

// -*-Mode: C++;-*-

//*****************************************************************************
// local includes
//*****************************************************************************

#include "level0-collector.hpp"
#include "level0-tracing-callbacks.hpp"

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

  // Create a new collector instance
  std::unique_ptr<ZeCollector> collector(new ZeCollector(dispatch));
  if (!collector) {
    EEMSG("Level0: Failed to allocate memory for ZeCollector");
    return nullptr;
  }

  // Notes(Yuning): Here we have two seperate mechanisms for callbacks
  // Create the tracer associated with the collector
  if (!level0CreateTracer(collector.get(), dispatch)) {
    EEMSG("Level0: Failed to create tracer for collector");
    // If tracer creation fails, the collector is automatically deleted
    return nullptr;
  }
  
  level0InitializeKernelBaseAddressFunction(dispatch);

  // Release ownership and return the raw pointer to the collector.
  return collector.release();
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
