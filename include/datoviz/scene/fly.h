/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/* Fly                                                                                           */
/*************************************************************************************************/

#ifndef DVZ_HEADER_FLY
#define DVZ_HEADER_FLY



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "datoviz_math.h"
#include "datoviz_types.h"
#include "scene/mvp.h"
#include "scene/panzoom.h"



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzFly DvzFly;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct DvzFly
{
    int flags;

    // Current state
    vec3 position; // Camera position
    float yaw;     // Rotation around Y axis (left/right)
    float pitch;   // Rotation around X axis (up/down)
    float roll;    // Rotation around Z axis (right mouse)

    // Initial state
    vec3 position_init;
    float yaw_init;
    float pitch_init;
    float roll_init;

    // Viewport size
    vec2 viewport_size;
} DvzFly;



#endif
