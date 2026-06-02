/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene axis label planning internals                                                          */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>

#include "_scene.h"



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct DvzAxisLabelPlan
{
    uint32_t tick_count;
    char tick_labels[DVZ_SCENE_MAX_AXIS_TICKS][DVZ_SCENE_LABEL_SIZE];
    bool has_offset_label;
    char offset_label[DVZ_SCENE_LABEL_SIZE];
} DvzAxisLabelPlan;



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

bool _axis_label_plan(
    const DvzAxis* axis, const double* ticks, uint32_t tick_count, double visible_min,
    double visible_max, DvzAxisLabelPlan* out);
