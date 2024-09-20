// SPDX-FileCopyrightText: 2002-2024 Rice University
// SPDX-FileCopyrightText: 2024 Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: BSD-3-Clause

// -*-Mode: C++;-*-

#ifndef LEVEL0_DRIVER_H_
#define LEVEL0_DRIVER_H_

//*****************************************************************************
// level zero includes
//*****************************************************************************

#include <level_zero/ze_api.h>


//*****************************************************************************
// system includes
//*****************************************************************************

#include <vector>


//*****************************************************************************
// local includes
//*****************************************************************************

#include "pti_assert.h"


//******************************************************************************
// interface operations
//******************************************************************************

void
zeroGetVersion
(
  ze_api_version_t& version
);


#endif // LEVEL0_DRIVER_H_