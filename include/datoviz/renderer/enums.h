/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Renderer enums                                                                               */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/

// Graphics builtins
// TODO: remove?
typedef enum
{
    DVZ_GRAPHICS_NONE,
    DVZ_GRAPHICS_POINT,
    DVZ_GRAPHICS_TRIANGLE,
    DVZ_GRAPHICS_CUSTOM,
} DvzGraphicsType;



/*************************************************************************************************/
/*  Renderer                                                                                     */
/*************************************************************************************************/

// Renderer flags.
typedef enum
{
    DVZ_RENDERER_FLAGS_NONE = 0x000000,
    DVZ_RENDERER_FLAGS_WHITE_BACKGROUND = 0x100000,
    DVZ_RENDERER_FLAGS_NO_WORKSPACE = 0x200000,
    DVZ_RENDERER_FLAGS_OFFSCREEN = 0x008000,
} DvzRendererFlags;
