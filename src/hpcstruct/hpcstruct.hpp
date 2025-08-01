// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: Apache-2.0

// -*-Mode: C++;-*- // technically C99

#ifndef hpcstruct_hpp
#define hpcstruct_hpp


#include "Args.hpp"
#include "Structure-Cache.hpp"

void doSingleBinary( Args &args, struct stat *sb);
void doMeasurementsDir ( Args &args, struct stat *sb);

#endif //hpcstruct_hpp
