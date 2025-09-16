// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: BSD-3-Clause

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