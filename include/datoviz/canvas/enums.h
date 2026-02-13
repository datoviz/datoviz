/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Canvas enums                                                                                 */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/

// Canvas creation flags.
typedef enum
{
    DVZ_CANVAS_FLAGS_NONE = 0x0000,
    DVZ_CANVAS_FLAGS_IMGUI = 0x0001,
    DVZ_CANVAS_FLAGS_FPS = 0x0003,     // NOTE: 1 bit for ImGUI, 1 bit for FPS
    DVZ_CANVAS_FLAGS_MONITOR = 0x0005, // NOTE: 1 bit for ImGUI, 1 bit for Monitor
    DVZ_CANVAS_FLAGS_FULLSCREEN = 0x0008,
    DVZ_CANVAS_FLAGS_VSYNC = 0x0010,
    DVZ_CANVAS_FLAGS_PICK = 0x0020,
    DVZ_CANVAS_FLAGS_PUSH_SCALE = 0x0040, // HACK: shaders expect a push constant with scaling
} DvzCanvasFlags;



// Canvas frame acquisition status codes.
typedef enum
{
    DVZ_CANVAS_FRAME_READY = 0,
    DVZ_CANVAS_FRAME_WAIT_SURFACE = 1,
} DvzCanvasFrameStatus;



// Canvas render mode.
typedef enum
{
    DVZ_CANVAS_RENDER_MODE_PRESENT = 0,
    DVZ_CANVAS_RENDER_MODE_OFFSCREEN = 1,
} DvzCanvasRenderMode;



// Present-mode runtime state diagnostics.
typedef enum
{
    DVZ_CANVAS_PRESENT_STATE_UNINITIALIZED = 0,
    DVZ_CANVAS_PRESENT_STATE_WAIT_SURFACE,
    DVZ_CANVAS_PRESENT_STATE_READY,
    DVZ_CANVAS_PRESENT_STATE_ACQUIRED,
    DVZ_CANVAS_PRESENT_STATE_PRESENT_PENDING,
    DVZ_CANVAS_PRESENT_STATE_FATAL_DEVICE_LOST,
} DvzCanvasPresentRuntimeState;



// Offscreen-mode runtime state diagnostics.
typedef enum
{
    DVZ_CANVAS_OFFSCREEN_STATE_UNINITIALIZED = 0,
    DVZ_CANVAS_OFFSCREEN_STATE_READY,
    DVZ_CANVAS_OFFSCREEN_STATE_DRAW_PENDING,
    DVZ_CANVAS_OFFSCREEN_STATE_OUTPUT_PENDING,
    DVZ_CANVAS_OFFSCREEN_STATE_FATAL_DEVICE_LOST,
} DvzCanvasOffscreenRuntimeState;
