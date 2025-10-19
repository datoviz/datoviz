/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Controller enums                                                                             */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/

// Arcball flags.
typedef enum
{
    DVZ_ARCBALL_FLAGS_NONE,
    DVZ_ARCBALL_FLAGS_CONSTRAIN,
} DvzArcballFlags;



// Fly flags.
typedef enum
{
    DVZ_FLY_FLAGS_NONE = 0x0000,
    DVZ_FLY_FLAGS_INVERT_MOUSE = 0x0001,
    DVZ_FLY_FLAGS_FIXED_UP = 0x0002,
} DvzFlyFlags;



// Panzoom flags.
typedef enum
{
    DVZ_PANZOOM_FLAGS_NONE = 0x00,
    DVZ_PANZOOM_FLAGS_KEEP_ASPECT = 0x01,
    DVZ_PANZOOM_FLAGS_FIXED_X = 0x10,
    DVZ_PANZOOM_FLAGS_FIXED_Y = 0x20,
} DvzPanzoomFlags;



// Camera flags.
typedef enum
{
    DVZ_CAMERA_FLAGS_PERSPECTIVE = 0x00,
    DVZ_CAMERA_FLAGS_ORTHO = 0x01,
} DvzCameraFlags;
