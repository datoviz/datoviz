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
    DvzScaleXY source_device_scale;
    float source_user_scale;
    uint64_t displayed_resource_generation;
    uint64_t source_frame_count;
    uint32_t stale_frame_count;
    bool has_frame;
    bool image_valid;
    bool display_ready;
    bool display_drawable;
} DvzGuiViewportDebugState;


typedef struct DvzGuiDataDebugState
{
    uint32_t row_count;
    uint32_t display_count;
    uint64_t rebuild_count;
} DvzGuiDataDebugState;


typedef struct DvzGuiScaleDebugState
{
    DvzScaleXY device;
    DvzScaleXY framebuffer;
    DvzScaleXY coordinate;
    float scalar_device;
    float scalar_framebuffer;
    float scalar_coordinate;
    float user;
} DvzGuiScaleDebugState;


typedef int (*DvzGuiViewportResolveCallback)(DvzView* view, void* user_data);



EXTERN_C_ON

/*************************************************************************************************/
/*  Internal functions                                                                           */
/*************************************************************************************************/

DvzGui*
_dvz_gui_create(
    DvzApp* app, DvzGpuCtx* gpu_ctx, DvzView* view, DvzWindow* window,
    const DvzGuiConfig* config, const DvzFontDefaults* font_defaults);
bool _dvz_gui_config_validate(const DvzGuiConfig* config);
void _dvz_gui_set_current(DvzGui* gui);
bool _dvz_gui_data_draw_context(DvzGui* gui, uint64_t* frame_index);
uint8_t _dvz_gui_srgb_to_linear_u8(uint8_t value);
DvzFontDefaults _dvz_gui_font_defaults(const DvzGui* gui);
void* _dvz_gui_bold_font(DvzGui* gui);
bool _dvz_gui_scale_resolve(
    DvzScaleXY device, DvzExtent native, DvzExtent framebuffer, float user,
    DvzGuiScaleDebugState* out);
void _dvz_gui_style_scale(float scale);
void _dvz_gui_destroy(DvzGui* gui);
void _dvz_gui_set_callback(DvzGui* gui, DvzGuiCallback callback, void* user_data);
void _dvz_gui_begin_frame(DvzGui* gui, DvzView* view, const DvzStreamFrame* frame);
bool _dvz_gui_resolve_viewports(
    DvzGui* gui, DvzGuiViewportResolveCallback callback, void* user_data);
void _dvz_gui_fps_overlay(
    DvzGui* gui, double fps, double frame_ms, uint32_t frames, double elapsed_s);
void _dvz_gui_render_frame(DvzGui* gui, const DvzStreamFrame* frame);
bool _dvz_gui_viewport_debug_state(
    const DvzGuiViewport* viewport, DvzGuiViewportDebugState* out);
bool _dvz_gui_tree_debug_state(DvzGuiTree* tree, DvzGuiDataDebugState* out);
uint64_t _dvz_gui_tree_debug_display_key(DvzGuiTree* tree, uint32_t display_index);
bool _dvz_gui_table_debug_state(DvzGuiTable* table, DvzGuiDataDebugState* out);
uint64_t _dvz_gui_table_debug_display_key(DvzGuiTable* table, uint32_t display_index);
DvzResult _dvz_gui_table_debug_sort(
    DvzGuiTable* table, uint32_t column_id, int32_t direction);

EXTERN_C_OFF
