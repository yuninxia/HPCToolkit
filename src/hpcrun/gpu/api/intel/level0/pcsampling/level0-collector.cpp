// SPDX-FileCopyrightText: 2002-2024 Rice University
// SPDX-FileCopyrightText: 2024 Contributors to the HPCToolkit Project
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
  level0CheckDriverVersion(1, 2, /*printVersion=*/false, dispatch);

  std::unique_ptr<ZeCollector> collector(new ZeCollector(data_dir, dispatch));

  // Create the tracer associated with the collector
  if (!level0CreateTracer(collector.get(), dispatch)) {
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
  level0EnumerateAndSetupDevices(dispatch);
  level0InitializeKernelCommandProperties();
}

ZeCollector::~ZeCollector()
{
  level0DestroyTracer(dispatch_);
  level0DumpKernelProfiles(data_dir_);
}
