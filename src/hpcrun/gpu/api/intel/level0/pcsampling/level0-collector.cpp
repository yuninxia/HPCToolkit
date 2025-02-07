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
  zeroCheckDriverVersion(1, 2, /*printVersion=*/false, dispatch);

  std::unique_ptr<ZeCollector> collector(new ZeCollector(data_dir, dispatch));

  // Create the tracer associated with the collector
  if (!zeroCreateTracer(collector.get(), dispatch)) {
    // If tracer creation fails, the collector is automatically deleted
    return nullptr;
  }
  
  zeroInitializeKernelBaseAddressFunction(dispatch);

  // Release ownership and return the raw pointer to the collector.
  return collector.release();
}

ZeCollector::ZeCollector
(
  const std::string& data_dir,
  const struct hpcrun_foil_appdispatch_level0* dispatch
) : data_dir_(data_dir), dispatch_(dispatch)
{
  zeroEnumerateAndSetupDevices(dispatch);
  zeroInitializeKernelCommandProperties();
}

ZeCollector::~ZeCollector()
{
  zeroDestroyTracer(dispatch_);
  zeroDumpKernelProfiles(data_dir_);
}
