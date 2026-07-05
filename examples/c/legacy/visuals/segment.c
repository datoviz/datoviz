/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* segment - live GLFW stress controls for segment visuals.
 *
 * Build:  just example-c visuals/segment
 * Run:    ./build/examples/c/visuals/segment
 * Smoke:  ./build/examples/c/visuals/segment 120
 */



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_compat.h"
#include "datoviz/app.h"
#include "datoviz/gui.h"
#include "datoviz/scene.h"
#include "example_common.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH  1100
#define HEIGHT 760

#define MAX_SEGMENT_COUNT 20000u

#define TWO_PI 6.28318530717958647692f



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/

typedef enum SegmentStressMode
{
    SEGMENT_STRESS_MODE_FIELD = 0,
    SEGMENT_STRESS_MODE_CROSSING = 1,
    SEGMENT_STRESS_MODE_RADIAL = 2,
    SEGMENT_STRESS_MODE_SHORT = 3,
} SegmentStressMode;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct SegmentStressState SegmentStressState;



struct SegmentStressState
{
    DvzView* win;
    DvzPanel* panel;
    DvzVisual* visual;

    uint32_t max_count;
    uint32_t active_count;
    float active_count_value;

    vec3* base_start;
    vec3* base_end;
    vec3* position_start;
    vec3* position_end;
    DvzColor* colors;
    float* stroke_widths;

    float stroke_width_px;
    float alpha;
    int count_preset;
    int start_cap;
    int end_cap;
    int mode;
    bool animate_endpoints;
    bool z_variation;
    bool show_demo;
    uint32_t frame_index;
};



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Return a bounded segment count from a float GUI value.
 *
 * @param value GUI count value
 * @param max_count maximum allowed segment count
 * @return clamped segment count
 */
static uint32_t _count_from_value(float value, uint32_t max_count)
{
    if (value < 1.0f)
        return 1;
    if (value > (float)max_count)
        return max_count;
    return (uint32_t)(value + 0.5f);
}



/**
 * Return one deterministic pseudo-random hash.
 *
 * @param value input integer
 * @return hashed integer
 */
static uint32_t _hash_u32(uint32_t value)
{
    value ^= value >> 16;
    value *= 0x7feb352du;
    value ^= value >> 15;
    value *= 0x846ca68bu;
    value ^= value >> 16;
    return value;
}



/**
 * Return a deterministic unit float for one segment and channel.
 *
 * @param index segment index
 * @param channel independent channel index
 * @return float in [0, 1]
 */
static float _unit(uint32_t index, uint32_t channel)
{
    const uint32_t hashed = _hash_u32(index * 747796405u + channel * 2891336453u + 277803737u);
    return (float)(hashed & 0x00ffffffu) / 16777215.0f;
}



/**
 * Convert a normalized float channel to an 8-bit color channel.
 *
 * @param value normalized channel value
 * @return clamped 8-bit channel value
 */
static uint8_t _u8_from_unit(float value)
{
    if (value < 0.0f)
        value = 0.0f;
    if (value > 1.0f)
        value = 1.0f;
    return (uint8_t)(255.0f * value + 0.5f);
}



/**
 * Return the segment cap matching one GUI combo index.
 *
 * @param index GUI combo index
 * @return segment cap enum
 */
static DvzSegmentCap _cap_from_index(int index)
{
    switch (index)
    {
    case 0:
        return DVZ_SEGMENT_CAP_NONE;
    case 1:
        return DVZ_SEGMENT_CAP_BUTT;
    case 2:
        return DVZ_SEGMENT_CAP_ROUND;
    case 3:
        return DVZ_SEGMENT_CAP_SQUARE;
    case 4:
        return DVZ_SEGMENT_CAP_TRIANGLE_IN;
    case 5:
        return DVZ_SEGMENT_CAP_TRIANGLE_OUT;
    default:
        return DVZ_SEGMENT_CAP_BUTT;
    }
}



/**
 * Fill one endpoint pair for the field stress mode.
 *
 * @param state segment stress state
 * @param i segment index
 */
static void _segment_field(SegmentStressState* state, uint32_t i)
{
    ANN(state);
    const float x = 1.82f * _unit(i, 0) - 0.91f;
    const float y = 1.58f * _unit(i, 1) - 0.79f;
    const float angle = TWO_PI * _unit(i, 2);
    const float length = 0.035f + 0.16f * _unit(i, 3);
    const float z = state->z_variation ? 0.42f * (_unit(i, 4) - 0.5f) : 0.0f;

    state->base_start[i][0] = x - 0.5f * length * cosf(angle);
    state->base_start[i][1] = y - 0.5f * length * sinf(angle);
    state->base_start[i][2] = z;
    state->base_end[i][0] = x + 0.5f * length * cosf(angle);
    state->base_end[i][1] = y + 0.5f * length * sinf(angle);
    state->base_end[i][2] = z;
}



/**
 * Fill one endpoint pair for the crossing stress mode.
 *
 * @param state segment stress state
 * @param i segment index
 */
static void _segment_crossing(SegmentStressState* state, uint32_t i)
{
    ANN(state);
    const float angle = TWO_PI * _unit(i, 0);
    const float radius = 0.72f + 0.28f * _unit(i, 1);
    const float skew = 0.30f * (_unit(i, 2) - 0.5f);
    const float z = state->z_variation ? 0.50f * (_unit(i, 3) - 0.5f) : 0.0f;

    state->base_start[i][0] = radius * cosf(angle);
    state->base_start[i][1] = radius * sinf(angle);
    state->base_start[i][2] = z;
    state->base_end[i][0] = -radius * cosf(angle + skew);
    state->base_end[i][1] = -radius * sinf(angle + skew);
    state->base_end[i][2] = z;
}



/**
 * Fill one endpoint pair for the radial stress mode.
 *
 * @param state segment stress state
 * @param i segment index
 */
static void _segment_radial(SegmentStressState* state, uint32_t i)
{
    ANN(state);
    const float angle = TWO_PI * _unit(i, 0);
    const float inner = 0.12f + 0.25f * _unit(i, 1);
    const float outer = 0.58f + 0.40f * _unit(i, 2);
    const float z = state->z_variation ? 0.34f * (_unit(i, 3) - 0.5f) : 0.0f;

    state->base_start[i][0] = inner * cosf(angle);
    state->base_start[i][1] = inner * sinf(angle);
    state->base_start[i][2] = z;
    state->base_end[i][0] = outer * cosf(angle);
    state->base_end[i][1] = outer * sinf(angle);
    state->base_end[i][2] = z;
}



/**
 * Fill one endpoint pair for the short-segment stress mode.
 *
 * @param state segment stress state
 * @param i segment index
 */
static void _segment_short(SegmentStressState* state, uint32_t i)
{
    ANN(state);
    const float x = 1.88f * _unit(i, 0) - 0.94f;
    const float y = 1.68f * _unit(i, 1) - 0.84f;
    const float angle = TWO_PI * _unit(i, 2);
    const float length = 0.006f + 0.034f * _unit(i, 3);
    const float z = state->z_variation ? 0.24f * (_unit(i, 4) - 0.5f) : 0.0f;

    state->base_start[i][0] = x - 0.5f * length * cosf(angle);
    state->base_start[i][1] = y - 0.5f * length * sinf(angle);
    state->base_start[i][2] = z;
    state->base_end[i][0] = x + 0.5f * length * cosf(angle);
    state->base_end[i][1] = y + 0.5f * length * sinf(angle);
    state->base_end[i][2] = z;
}



/**
 * Rebuild deterministic base endpoint data for the current mode.
 *
 * @param state segment stress state
 */
static void _segment_rebuild_base(SegmentStressState* state)
{
    ANN(state);
    for (uint32_t i = 0; i < state->max_count; i++)
    {
        if (state->mode == SEGMENT_STRESS_MODE_CROSSING)
            _segment_crossing(state, i);
        else if (state->mode == SEGMENT_STRESS_MODE_RADIAL)
            _segment_radial(state, i);
        else if (state->mode == SEGMENT_STRESS_MODE_SHORT)
            _segment_short(state, i);
        else
            _segment_field(state, i);
    }
}



/**
 * Fill derived endpoints, colors, and widths for upload.
 *
 * @param state segment stress state
 */
static void _segment_fill_upload_buffers(SegmentStressState* state)
{
    ANN(state);
    const float phase = 0.026f * (float)state->frame_index;
    const uint8_t alpha = _u8_from_unit(state->alpha);

    for (uint32_t i = 0; i < state->active_count; i++)
    {
        float dx = 0.0f;
        float dy = 0.0f;
        if (state->animate_endpoints)
        {
            dx = 0.026f * sinf(phase + TWO_PI * _unit(i, 5));
            dy = 0.026f * cosf(0.83f * phase + TWO_PI * _unit(i, 6));
        }

        state->position_start[i][0] = state->base_start[i][0] - dx;
        state->position_start[i][1] = state->base_start[i][1] - dy;
        state->position_start[i][2] = state->base_start[i][2];
        state->position_end[i][0] = state->base_end[i][0] + dx;
        state->position_end[i][1] = state->base_end[i][1] + dy;
        state->position_end[i][2] = state->base_end[i][2];

        state->colors[i] = dvz_color_rgba(
            _u8_from_unit(0.25f + 0.70f * _unit(i, 7)),
            _u8_from_unit(0.35f + 0.55f * _unit(i, 8)),
            _u8_from_unit(0.45f + 0.50f * _unit(i, 9)), alpha);
        state->stroke_widths[i] = state->stroke_width_px * (0.55f + 0.90f * _unit(i, 10));
    }
}



/**
 * Upload the current segment visual parameters.
 *
 * @param state segment stress state
 */
static void _segment_upload(SegmentStressState* state)
{
    ANN(state);
    if (state->visual == NULL)
        return;

    state->active_count = _count_from_value(state->active_count_value, state->max_count);
    state->active_count_value = (float)state->active_count;
    _segment_fill_upload_buffers(state);

    DvzVisualDataUpdate updates[4] = {
        {
            .attr_name = "position_start",
            .data = state->position_start,
            .item_count = state->active_count,
        },
        {
            .attr_name = "position_end",
            .data = state->position_end,
            .item_count = state->active_count,
        },
        {
            .attr_name = "color",
            .data = state->colors,
            .item_count = state->active_count,
        },
        {
            .attr_name = "stroke_width_px",
            .data = state->stroke_widths,
            .item_count = state->active_count,
        },
    };
    if (dvz_visual_set_data_many(state->visual, updates, 4) != 0)
        dvz_fprintf(stderr, "dvz_visual_set_data_many() failed\n");
    dvz_segment_set_caps(
        state->visual, _cap_from_index(state->start_cap), _cap_from_index(state->end_cap));
    dvz_visual_set_alpha_mode(
        state->visual, state->alpha < 0.999f ? DVZ_ALPHA_BLENDED : DVZ_ALPHA_OPAQUE);

    if (state->win != NULL)
        dvz_view_request_frame(state->win);
}



/**
 * Reset GUI-editable stress controls.
 *
 * @param state segment stress state
 */
static void _segment_reset(SegmentStressState* state)
{
    ANN(state);
    state->active_count = 8192;
    state->active_count_value = (float)state->active_count;
    state->stroke_width_px = 4.0f;
    state->alpha = 0.82f;
    state->count_preset = 3;
    state->start_cap = 1;
    state->end_cap = 2;
    state->mode = SEGMENT_STRESS_MODE_FIELD;
    state->animate_endpoints = true;
    state->z_variation = false;
    state->show_demo = false;
    state->frame_index = 0;
    _segment_rebuild_base(state);
    _segment_upload(state);
}



/**
 * Release heap buffers owned by the stress state.
 *
 * @param state segment stress state
 */
static void _segment_state_destroy(SegmentStressState* state)
{
    if (state == NULL)
        return;

    dvz_free(state->base_start);
    dvz_free(state->base_end);
    dvz_free(state->position_start);
    dvz_free(state->position_end);
    dvz_free(state->colors);
    dvz_free(state->stroke_widths);
    state->base_start = NULL;
    state->base_end = NULL;
    state->position_start = NULL;
    state->position_end = NULL;
    state->colors = NULL;
    state->stroke_widths = NULL;
}



/**
 * Allocate heap buffers owned by the stress state.
 *
 * @param state segment stress state
 * @return whether allocation succeeded
 */
static bool _segment_state_alloc(SegmentStressState* state)
{
    ANN(state);
    state->base_start = (vec3*)dvz_calloc(state->max_count, sizeof(*state->base_start));
    state->base_end = (vec3*)dvz_calloc(state->max_count, sizeof(*state->base_end));
    state->position_start =
        (vec3*)dvz_calloc(state->max_count, sizeof(*state->position_start));
    state->position_end = (vec3*)dvz_calloc(state->max_count, sizeof(*state->position_end));
    state->colors = (DvzColor*)dvz_calloc(state->max_count, sizeof(DvzColor));
    state->stroke_widths = (float*)dvz_calloc(state->max_count, sizeof(float));

    if (state->base_start == NULL || state->base_end == NULL || state->position_start == NULL ||
        state->position_end == NULL || state->colors == NULL || state->stroke_widths == NULL)
    {
        _segment_state_destroy(state);
        return false;
    }
    return true;
}



/**
 * Apply a selected count preset.
 *
 * @param state segment stress state
 */
static void _segment_apply_count_preset(SegmentStressState* state)
{
    ANN(state);
    switch (state->count_preset)
    {
    case 1:
        state->active_count_value = 512.0f;
        break;
    case 2:
        state->active_count_value = 2048.0f;
        break;
    case 3:
        state->active_count_value = 8192.0f;
        break;
    case 4:
        state->active_count_value = (float)state->max_count;
        break;
    default:
        break;
    }
}



/**
 * Build the dockable segment stress GUI.
 *
 * @param gui GUI overlay
 * @param win view
 * @param user_data segment stress state
 */
static void _segment_gui(DvzGui* gui, DvzView* win, void* user_data)
{
    SegmentStressState* state = (SegmentStressState*)user_data;
    ANN(state);
    state->win = win;
    bool changed = false;
    bool rebuild = false;

    if (dvz_gui_begin(gui, "Segment stress", NULL, 0))
    {
        static const char* const count_items[] = {
            "custom",
            "512",
            "2048",
            "8192",
            "20000",
        };
        static const char* const cap_items[] = {
            "none",
            "butt",
            "round",
            "square",
            "triangle in",
            "triangle out",
        };
        static const char* const mode_items[] = {
            "field",
            "crossing",
            "radial",
            "short",
        };

        if (dvz_gui_combo(gui, "Count preset", &state->count_preset, count_items, 5))
        {
            _segment_apply_count_preset(state);
            changed = true;
        }
        if (dvz_gui_slider_float_format(
                gui, "Active count", &state->active_count_value, 1.0f, (float)state->max_count,
                "%.0f"))
        {
            state->count_preset = 0;
            changed = true;
        }
        changed |=
            dvz_gui_slider_float_format(gui, "Stroke width", &state->stroke_width_px, 0.5f, 30.0f,
                                        "%.1f px");
        changed |= dvz_gui_combo(gui, "Start cap", &state->start_cap, cap_items, 6);
        changed |= dvz_gui_combo(gui, "End cap", &state->end_cap, cap_items, 6);
        if (dvz_gui_combo(gui, "Endpoint mode", &state->mode, mode_items, 4))
        {
            rebuild = true;
            changed = true;
        }
        changed |= dvz_gui_slider_float(gui, "Alpha", &state->alpha, 0.02f, 1.0f);
        changed |= dvz_gui_checkbox(gui, "Animate endpoints", &state->animate_endpoints);
        if (dvz_gui_checkbox(gui, "Z variation", &state->z_variation))
        {
            rebuild = true;
            changed = true;
        }
        (void)dvz_gui_checkbox(gui, "ImGui demo", &state->show_demo);
        if (dvz_gui_button(gui, "Reset"))
        {
            _segment_reset(state);
            changed = false;
            rebuild = false;
        }
    }
    dvz_gui_end(gui);

    if (state->show_demo)
        dvz_gui_demo(gui, &state->show_demo);

    if (rebuild)
        _segment_rebuild_base(state);
    if (changed)
        _segment_upload(state);
}



/**
 * Advance optional endpoint animation after a rendered frame.
 *
 * @param win view
 * @param user_data segment stress state
 */
static void _segment_frame(DvzView* win, void* user_data)
{
    (void)win;
    SegmentStressState* state = (SegmentStressState*)user_data;
    ANN(state);
    if (!state->animate_endpoints)
        return;

    state->frame_index++;
    _segment_upload(state);
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Run the live segment visual stress example.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @return process exit code
 */
int main(int argc, char** argv)
{
    int exit_code = 1;
    DvzScene* scene = NULL;
    DvzApp* app = NULL;
    SegmentStressState state = {
        .max_count = MAX_SEGMENT_COUNT,
    };

    bool ok = _segment_state_alloc(&state);
    EXAMPLE_CHECK(ok, "segment stress buffer allocation failed");

    scene = dvz_scene();
    EXAMPLE_CHECK(scene != NULL, "dvz_scene() failed");

    DvzFigure* figure = dvz_figure(scene, WIDTH, HEIGHT, 0);
    EXAMPLE_CHECK(figure != NULL, "dvz_figure() failed");

    state.panel = dvz_panel_full(figure);
    EXAMPLE_CHECK(state.panel != NULL, "dvz_panel() failed");

    state.visual = dvz_segment(scene, 0);
    EXAMPLE_CHECK(state.visual != NULL, "dvz_segment() failed");

    (void)dvz_visual_set_attr_mutability(
        state.visual, "position_start", DVZ_VISUAL_ATTR_MUTABILITY_STREAMING);
    (void)dvz_visual_set_attr_mutability(
        state.visual, "position_end", DVZ_VISUAL_ATTR_MUTABILITY_STREAMING);
    (void)dvz_visual_set_attr_mutability(
        state.visual, "color", DVZ_VISUAL_ATTR_MUTABILITY_STREAMING);
    (void)dvz_visual_set_attr_mutability(
        state.visual, "stroke_width_px", DVZ_VISUAL_ATTR_MUTABILITY_STREAMING);

    _segment_reset(&state);
    int rc = dvz_panel_add_visual(state.panel, state.visual, NULL);
    EXAMPLE_CHECK(rc == 0, "dvz_panel_add_visual() failed");
    dvz_panel_set_background_color(state.panel, dvz_color_from_unit(0.055f, 0.060f, 0.075f, 1.0f));

    app = dvz_app(scene);
    EXAMPLE_CHECK(app != NULL, "dvz_app() failed (no GPU or display?)");

    state.win = dvz_view_window(app, figure, WIDTH, HEIGHT, "segment");
    EXAMPLE_CHECK(state.win != NULL, "dvz_view_window() failed (GLFW unavailable?)");

    DvzPanzoom* panzoom = dvz_view_panzoom(state.win, state.panel, NULL);
    EXAMPLE_CHECK(panzoom != NULL, "failed to create or bind panzoom controller");

    DvzGui* gui = dvz_view_gui(state.win, NULL);
    EXAMPLE_CHECK(gui != NULL, "dvz_view_gui() failed");
    dvz_view_set_gui_callback(state.win, _segment_gui, &state);
    dvz_view_set_frame_callback(state.win, _segment_frame, &state);
    dvz_scene_set_clock_mode(scene, DVZ_SCENE_CLOCK_REALTIME);
    dvz_scene_set_fps(scene, 60.0);

    dvz_app_run(app, example_frame_count(argc, argv));
    exit_code = 0;

cleanup:
    if (app != NULL)
        dvz_app_destroy(app);
    if (scene != NULL)
        dvz_scene_destroy(scene);
    _segment_state_destroy(&state);
    return exit_code;
}
