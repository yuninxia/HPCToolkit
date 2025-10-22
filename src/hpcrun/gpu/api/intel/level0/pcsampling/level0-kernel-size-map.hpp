// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: Apache-2.0
//
// SPDX-License-Identifier: MIT
// Copyright (C) 2024 Intel Corporation
// This file was inspired by and uses some code fragments from Intel's
// MIT-licensed pti-gpu (https://github.com/intel/pti-gpu)

// -*-Mode: C++;-*-

#ifndef KERNEL_SIZE_MAP_HPP
#define KERNEL_SIZE_MAP_HPP

//*****************************************************************************
// system includes
//*****************************************************************************

#include <string>
#include <unordered_map>


//*****************************************************************************
// local includes
//*****************************************************************************

#include "../level0-id-map.h"


//******************************************************************************
// interface operations
//******************************************************************************

void
level0FillKernelSizeMap
(
  zebin_id_map_entry_t *entry
);

size_t
level0GetKernelSize
(
  const std::string& kernel_name
);


#endif // KERNEL_SIZE_MAP_HPP
