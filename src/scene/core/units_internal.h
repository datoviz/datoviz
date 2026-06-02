/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene units and datetime internals                                                           */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stddef.h>

#include "_scene.h"



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct DvzUnitFormatContext
{
    DvzUnitDisplayMode mode;
    bool has_axis_range;
    double axis_data_min;
    double axis_data_max;
} DvzUnitFormatContext;



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

bool _scene_units_format(
    const DvzUnits* units, double data_value, const DvzUnitFormatContext* context, char* out,
    size_t out_size);

bool _scene_datetime_format(
    const DvzDateTimeFormat* format, DvzTimestamp timestamp, DvzTimeInterval interval, char* out,
    size_t out_size);

DvzTimestamp _scene_datetime_data_to_timestamp(const DvzAxis* axis, double value);

double _scene_datetime_timestamp_to_data(const DvzAxis* axis, DvzTimestamp timestamp);
