// SPDX-FileCopyrightText: 2002-2024 Rice University
// SPDX-FileCopyrightText: 2024 Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: BSD-3-Clause

// -*-Mode: C++;-*-

//*****************************************************************************
// local includes
//*****************************************************************************

#include "level0-unique-id.hpp"


//******************************************************************************
// interface operations
//******************************************************************************

std::string 
zeroGenerateUniqueId
(
  const void *data,
  size_t binary_size
)
{
  char hash_string[CRYPTO_HASH_STRING_LENGTH] = {0};
  crypto_compute_hash_string(data, binary_size, hash_string, CRYPTO_HASH_STRING_LENGTH);
  return std::string(hash_string);
}
