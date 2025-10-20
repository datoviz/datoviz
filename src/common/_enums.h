/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Common enums                                                                                 */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define DVZ_MAX_BINDINGS        16
#define DVZ_MAX_VERTEX_ATTRS    16
#define DVZ_MAX_VERTEX_BINDINGS 8
#define DVZ_MAX_PARAMS          16



/*************************************************************************************************/
/*  Resources                                                                                    */
/*************************************************************************************************/

// Default queue.
typedef enum
{
    // NOTE: by convention in vklite, the first queue MUST support transfers
    DVZ_DEFAULT_QUEUE_TRANSFER,
    DVZ_DEFAULT_QUEUE_COMPUTE,
    DVZ_DEFAULT_QUEUE_RENDER,
    DVZ_DEFAULT_QUEUE_PRESENT,
    DVZ_DEFAULT_QUEUE_COUNT,
} DvzDefaultQueue;



// Dat usage.
// TODO: not implemented yet, going from these flags to DvzDatFlags
typedef enum
{
    DVZ_DAT_USAGE_FREQUENT_NONE,
    DVZ_DAT_USAGE_FREQUENT_UPLOAD = 0x0001,
    DVZ_DAT_USAGE_FREQUENT_DOWNLOAD = 0x0002,
    DVZ_DAT_USAGE_FREQUENT_RESIZE = 0x0004,
} DvzDatUsage;



/*************************************************************************************************/
/*  Graphics                                                                                     */
/*************************************************************************************************/

// Graphics flags.
typedef enum
{
    DVZ_GRAPHICS_FLAGS_DEPTH_TEST = 0x40000,
    DVZ_GRAPHICS_FLAGS_PICK = 0x80000,
} DvzGraphicsFlags;



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



/*************************************************************************************************/
/*  Scene                                                                                        */
/*************************************************************************************************/

// Build status
typedef enum
{
    DVZ_BUILD_CLEAR,
    DVZ_BUILD_BUSY,
    DVZ_BUILD_DIRTY,
} DvzBuildStatus;



// Panel resizing.
typedef enum
{
    DVZ_PANEL_RESIZE_STRETCH = 0x00,

    DVZ_PANEL_RESIZE_FIXED_WIDTH = 0x01,
    DVZ_PANEL_RESIZE_FIXED_HEIGHT = 0x02,

    // if these 2 flags are set, FIXED_WIDTH should not be set
    DVZ_PANEL_RESIZE_FIXED_TOP_LEFT = 0x10,
    DVZ_PANEL_RESIZE_FIXED_TOP_RIGHT = 0x20,

    // if these 2 flags are set, FIXED_HEIGHT should not be set
    DVZ_PANEL_RESIZE_FIXED_BOTTOM_LEFT = 0x40,
    DVZ_PANEL_RESIZE_FIXED_BOTTOM_RIGHT = 0x80,

    DVZ_PANEL_RESIZE_FIXED_SHAPE = 0x03,
    DVZ_PANEL_RESIZE_FIXED_OFFSET = 0xF0,

    DVZ_PANEL_RESIZE_FIXED = 0xF3,
} DvzPanelResizing;
