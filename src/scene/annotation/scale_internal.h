/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */


/*************************************************************************************************/
/*  Scene scale annotation internals                                                             */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "_scene.h"



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

void _scene_mark_scale_dirty(DvzScale* scale);

void _scene_mark_legend_dirty(DvzLegend* legend);
