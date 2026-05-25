/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene scale tick helpers                                                                     */
/*************************************************************************************************/

#pragma once

#include <stdbool.h>
#include <stddef.h>



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

double _scene_nice_number(double value, bool round_to_nearest);

bool _scene_scalebar_choose_length(
    double units_per_px, float target_px, float min_px, float max_px, double* out_units,
    float* out_px);

void _scene_format_si_value(double value, const char* unit, char* out, size_t out_size);
