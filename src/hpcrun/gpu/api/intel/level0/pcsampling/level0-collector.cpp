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

  ZeCollector* collector = new ZeCollector(data_dir, dispatch);
  bool tracerCreated = zeroCreateTracer(collector, dispatch);
  if (!tracerCreated) {
    delete collector;
    return nullptr;
  }
  
  zeroInitializeKernelBaseAddressFunction(dispatch);

  return collector;
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
