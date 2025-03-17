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
  const std::string& data_dir,
  const struct hpcrun_foil_appdispatch_level0* dispatch
)
{
  // Verify that the driver version meets minimum requirements
  if (!level0CheckDriverVersion(1, 2, /*printVersion=*/false, dispatch)) {
    std::cerr << "Failed to create collector: driver version requirements not met" << std::endl;
    return nullptr;
  }

  // Create a new collector instance
  std::unique_ptr<ZeCollector> collector(new ZeCollector(data_dir, dispatch));
  if (!collector) {
    std::cerr << "Failed to allocate memory for ZeCollector" << std::endl;
    return nullptr;
  }

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
  const std::string& data_dir,
  const struct hpcrun_foil_appdispatch_level0* dispatch
) : data_dir_(data_dir), dispatch_(dispatch)
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
  
  // Dump collected kernel profiles to the data directory
  level0DumpKernelProfiles(data_dir_);
}
