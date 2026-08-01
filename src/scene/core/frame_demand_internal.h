/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene frame demand                                                                          */
/*************************************************************************************************/

#pragma once

#include <stdint.h>

#include "_scene.h"



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/

typedef enum DvzFrameDemandFlags
{
    DVZ_FRAME_DEMAND_NONE = 0,
    DVZ_FRAME_DEMAND_INTERACTION = 1 << 0,
} DvzFrameDemandFlags;



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

bool _scene_fly_active(const DvzFly* fly);

DvzFrameDemandFlags _scene_figure_frame_demand(const DvzFigure* figure);
