// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
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
level0FillKernelSizeMap
(
  zebin_id_map_entry_t *entry
);

size_t
level0GetKernelSize
(
  std::string& kernel_name
);


#endif // KERNEL_SIZE_MAP_HPP
