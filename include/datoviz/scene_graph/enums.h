/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene graph enums                                                                            */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/

// Blend type.
typedef enum
{
    DVZ_BLEND_DISABLE,
    DVZ_BLEND_STANDARD,
    DVZ_BLEND_DESTINATION,
    DVZ_BLEND_OIT,
} DvzBlendType;



// Slot type.
typedef enum
{
    DVZ_SLOT_DAT,
    DVZ_SLOT_TEX,
    DVZ_SLOT_COUNT,
} DvzSlotType;



// Depth test.
typedef enum
{
    DVZ_DEPTH_TEST_DISABLE,
    DVZ_DEPTH_TEST_ENABLE,
} DvzDepthTest;



// Orientation.
typedef enum
{
    DVZ_ORIENTATION_DEFAULT,
    DVZ_ORIENTATION_UP,
    DVZ_ORIENTATION_REVERSE,
    DVZ_ORIENTATION_DOWN,
} DvzOrientation;



// Alignment.
typedef enum
{
    DVZ_ALIGN_NONE,
    DVZ_ALIGN_LOW,
    DVZ_ALIGN_MIDDLE,
    DVZ_ALIGN_HIGH,
} DvzAlign;



// NOTE: must correspond to values in common.glsl
typedef enum
{
    DVZ_VIEWPORT_CLIP_INNER = 0x0001,
    DVZ_VIEWPORT_CLIP_OUTER = 0x0002,
    DVZ_VIEWPORT_CLIP_BOTTOM = 0x0004,
    DVZ_VIEWPORT_CLIP_LEFT = 0x0008,
} DvzViewportClip;



// Visual flags.
// NOTE: these flags are also passed to BakerFlags
typedef enum
{
    DVZ_VISUAL_FLAGS_DEFAULT = 0x000000,
    DVZ_VISUAL_FLAGS_INDEXED = 0x010000,
    DVZ_VISUAL_FLAGS_INDIRECT = 0x020000,

    DVZ_VISUAL_FLAGS_FIXED_X = 0x001000,
    DVZ_VISUAL_FLAGS_FIXED_Y = 0x002000,
    DVZ_VISUAL_FLAGS_FIXED_Z = 0x004000,
    DVZ_VISUAL_FLAGS_FIXED_ALL = 0x007000,

    DVZ_VISUAL_FLAGS_VERTEX_MAPPABLE = 0x400000,
    DVZ_VISUAL_FLAGS_INDEX_MAPPABLE = 0x800000,
} DvzVisualFlags;



// View flags (panel_visual).
typedef enum
{
    DVZ_VIEW_FLAGS_NONE = 0x0000,
    DVZ_VIEW_FLAGS_STATIC = 0x0010,
    DVZ_VIEW_FLAGS_NOCLIP = 0x0020,
} DvzViewFlags;



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
