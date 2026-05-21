/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene controller internals                                                                   */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "datoviz/scene/arcball.h"
#include "datoviz/scene/fly.h"
#include "datoviz/scene/panzoom.h"
#include "datoviz/scene/turntable.h"



EXTERN_C_ON

/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

DvzPanzoom* _dvz_panzoom(float width, float height, int flags);

DvzArcball* _dvz_arcball(float width, float height, int flags);

DvzFly* _dvz_fly(const DvzFlyDesc* desc);

DvzTurntable* _dvz_turntable(const DvzTurntableDesc* desc);



EXTERN_C_OFF
