// SPDX-FileCopyrightText: 2002-2024 Rice University
// SPDX-FileCopyrightText: 2024 Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: BSD-3-Clause

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
zeroFillKernelSizeMap
(
  zebin_id_map_entry_t *entry
);

size_t
zeroGetKernelSize
(
  std::string& kernel_name
);


#endif // KERNEL_SIZE_MAP_HPP
