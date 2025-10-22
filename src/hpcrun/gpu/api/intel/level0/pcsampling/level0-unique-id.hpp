// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: Apache-2.0
//
// SPDX-License-Identifier: MIT
// Copyright (C) 2024 Intel Corporation
// This file was inspired by and uses some code fragments from Intel's
// MIT-licensed pti-gpu (https://github.com/intel/pti-gpu)

// -*-Mode: C++;-*-

#ifndef LEVEL0_UNIQUE_ID_HPP
#define LEVEL0_UNIQUE_ID_HPP

//*****************************************************************************
// system includes
//*****************************************************************************

#include <iostream>
#include <string>


//******************************************************************************
// local includes
//******************************************************************************

#include "../../../../../../common/lean/crypto-hash.h"


//******************************************************************************
// interface operations
//******************************************************************************

std::string
level0GenerateUniqueId
(
  const void *data,
  size_t binary_size
);

#endif // LEVEL0_UNIQUE_ID_HPP
