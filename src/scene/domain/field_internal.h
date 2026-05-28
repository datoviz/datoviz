/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene field internals                                                                        */
/*************************************************************************************************/

#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "_scene.h"

bool _field_format_supported(DvzFieldFormat format);

bool _field_format_is_rgba8(DvzFieldFormat format);

bool _field_expected_data_size(const DvzSampledFieldDesc* desc, uint64_t* out_size);

uint64_t _field_default_bytes_per_row(const DvzSampledFieldDesc* desc);

uint64_t _field_default_rows_per_image(const DvzSampledFieldDesc* desc);

DvzFieldRegion _field_full_region(const DvzSampledFieldDesc* desc);

bool _field_regions_union(
    const DvzFieldRegion* a, const DvzFieldRegion* b, DvzFieldRegion* out);
