/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Internal Dear ImGui overlay                                                                  */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "datoviz/gui.h"
#include "datoviz/stream/frame_stream.h"
#include "datoviz/window.h"



/*************************************************************************************************/
/*  Forward declarations                                                                         */
/*************************************************************************************************/

typedef struct DvzGpuCtx DvzGpuCtx;



typedef struct DvzGuiViewportDebugState
{
    uint32_t requested_width;
    uint32_t requested_height;
    uint32_t requested_framebuffer_width;
    uint32_t requested_framebuffer_height;
    uint32_t pending_width;
    uint32_t pending_height;
    uint32_t pending_stable_frames;
    uint32_t displayed_framebuffer_width;
    uint32_t displayed_framebuffer_height;
    uint32_t stale_frame_count;
    bool has_frame;
    bool display_ready;
} DvzGuiViewportDebugState;



EXTERN_C_ON

/*************************************************************************************************/
/*  Internal functions                                                                           */
/*************************************************************************************************/

DvzGui*
_dvz_gui_create(
    DvzApp* app, DvzGpuCtx* gpu_ctx, DvzView* view, DvzWindow* window,
    const DvzGuiConfig* config);
bool _dvz_gui_config_validate(const DvzGuiConfig* config);
void _dvz_gui_destroy(DvzGui* gui);
void _dvz_gui_set_callback(DvzGui* gui, DvzGuiCallback callback, void* user_data);
void _dvz_gui_begin_frame(DvzGui* gui, DvzView* view, const DvzStreamFrame* frame);
void _dvz_gui_fps_overlay(
    DvzGui* gui, double fps, double frame_ms, uint32_t frames, double elapsed_s);
void _dvz_gui_render_frame(DvzGui* gui, const DvzStreamFrame* frame);
bool _dvz_gui_viewport_debug_state(
    const DvzGuiViewport* viewport, DvzGuiViewportDebugState* out);

EXTERN_C_OFF
