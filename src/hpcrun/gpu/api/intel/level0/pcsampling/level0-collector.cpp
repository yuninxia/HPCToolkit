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
  const std::string& data_dir
)
{
  zeroCheckDriverVersion(1, 2, /*printVersion=*/false);

  ZeCollector* collector = new ZeCollector(data_dir);
  zel_tracer_handle_t tracer = zeroCreateTracer(collector);
  if (tracer == nullptr) {
    delete collector;
    return nullptr;
  }

  zeroEnableTracing(tracer);
  
  zeroInitializeKernelBaseAddressFunction();

  return collector;
}

ZeCollector::ZeCollector
(
  const std::string& data_dir
) : data_dir_(data_dir)
{
  zeroEnumerateAndSetupDevices();
  zeroInitializeKernelCommandProperties();
}

ZeCollector::~ZeCollector()
{
  zeroDestroyTracer(tracer_);
  zeroDumpKernelProfiles(data_dir_);
}
