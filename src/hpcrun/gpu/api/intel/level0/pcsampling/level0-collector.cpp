// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: BSD-3-Clause

// -*-Mode: C++;-*-

//*****************************************************************************
// local includes
//*****************************************************************************

#include "level0-collector.hpp"
#include "level0-tracing-callbacks.hpp"


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
    std::cerr << "Failed to create collector: driver version requirements not met" << std::endl;
    return nullptr;
  }

  // Create a new collector instance
  std::unique_ptr<ZeCollector> collector(new ZeCollector(dispatch));
  if (!collector) {
    std::cerr << "Failed to allocate memory for ZeCollector" << std::endl;
    return nullptr;
  }

  // Notes(Yuning): Here we have two seperate mechanisms for callbacks
  // Create the tracer associated with the collector
  if (!level0CreateTracer(collector.get(), dispatch)) {
    std::cerr << "Failed to create tracer for collector" << std::endl;
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
  
  // Initialize properties for kernel commands
  level0InitializeKernelCommandProperties();
}

ZeCollector::~ZeCollector()
{  
  // Clean up tracer resources
  level0DestroyTracer(dispatch_);
  
  // Dump collected kernel profiles
  level0DumpKernelProfiles();
}
