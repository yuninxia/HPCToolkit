// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
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
level0GenerateUniqueId
(
  const void *data,
  size_t binary_size
)
{
  if (data == nullptr) {
    std::cerr << "[WARNING] Null data pointer passed to level0GenerateUniqueId" << std::endl;
    return "invalid_hash_null_data";
  }

  if (binary_size == 0) {
    std::cerr << "[WARNING] Zero binary size passed to level0GenerateUniqueId" << std::endl;
    return "invalid_hash_zero_size";
  }

  char hash_string[CRYPTO_HASH_STRING_LENGTH] = {0};
  if (crypto_compute_hash_string(data, binary_size, hash_string, CRYPTO_HASH_STRING_LENGTH) != 0) {
    std::cerr << "[WARNING] Failed to compute hash in level0GenerateUniqueId" << std::endl;
    return "hash_computation_failed";
  }
  
  return std::string(hash_string);
}
