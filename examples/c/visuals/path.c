/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* path - live GLFW stress tool for retained path visuals.
 *
 * Build:  just example-c visuals/path
 * Run:    ./build/examples/c/visuals/path
 * Smoke:  ./build/examples/c/visuals/path 300
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
#include "datoviz/imgui.h"
#include "datoviz/scene.h"
#include "example_common.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH                 1200u
#define HEIGHT                760u
#define PATH_MAX_VERTICES     65536u
#define PATH_MIN_VERTICES     2u
#define PATH_DEFAULT_VERTICES 4096u
#define PATH_MAX_SUBPATHS     512u

static const float TAU = 6.28318530718f;



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/

typedef enum PathStressSubpathMode
{
    PATH_STRESS_SUBPATH_SINGLE = 0,
    PATH_STRESS_SUBPATH_EVEN = 1,
    PATH_STRESS_SUBPATH_STAGGERED = 2,
} PathStressSubpathMode;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct PathStressState
{
    DvzVisual* thin_visual;
    DvzVisual* stroked_visual;

    float (*positions)[3];
    DvzColor* colors;
    float* stroke_widths;
    uint32_t* subpath_lengths;

    uint32_t active_count;
    uint32_t subpath_count;
    PathStressSubpathMode subpath_mode;

    bool stroked_path;
    bool per_point_width;
    bool animated_deformation;
    bool color_gradient;
    bool high_frequency_zigzag;

    float global_stroke_width;
    float phase;
    uint64_t frame_index;
} PathStressState;



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Clamp one unsigned value to an inclusive range.
 *
 * @param value input value
 * @param min minimum value
 * @param max maximum value
 * @return clamped value
 */
static uint32_t _clamp_u32(uint32_t value, uint32_t min, uint32_t max)
{
    if (value < min)
        return min;
    if (value > max)
        return max;
    return value;
}



/**
 * Clamp one float to an inclusive range.
 *
 * @param value input value
 * @param min minimum value
 * @param max maximum value
 * @return clamped value
 */
static float _clamp_float(float value, float min, float max)
{
    if (value < min)
        return min;
    if (value > max)
        return max;
    return value;
}



/**
 * Return the effective subpath count for the current state.
 *
 * @param state example state
 * @return effective subpath count
 */
static uint32_t _effective_subpath_count(const PathStressState* state)
{
    ANN(state);
    if (state->subpath_mode == PATH_STRESS_SUBPATH_SINGLE)
        return 1;
    uint32_t max_count = state->active_count < PATH_MAX_SUBPATHS ? state->active_count :
                                                                  PATH_MAX_SUBPATHS;
    return _clamp_u32(state->subpath_count, 1, max_count);
}



/**
 * Rebuild deterministic subpath lengths that sum to the active vertex count.
 *
 * @param state example state
 * @return effective subpath count
 */
static uint32_t _build_subpaths(PathStressState* state)
{
    ANN(state);
    ANN(state->subpath_lengths);

    state->active_count = _clamp_u32(state->active_count, PATH_MIN_VERTICES, PATH_MAX_VERTICES);
    uint32_t count = _effective_subpath_count(state);

    if (count == 1 || state->subpath_mode == PATH_STRESS_SUBPATH_EVEN)
    {
        uint32_t base = state->active_count / count;
        uint32_t rem = state->active_count % count;
        for (uint32_t i = 0; i < count; i++)
            state->subpath_lengths[i] = base + (i < rem ? 1u : 0u);
        return count;
    }

    uint32_t remaining = state->active_count;
    uint32_t remaining_paths = count;
    for (uint32_t i = 0; i < count; i++)
    {
        uint32_t min_tail = remaining_paths - 1u;
        uint32_t target = 3u + ((i * 17u + 11u) % 29u);
        if ((i % 5u) == 0u)
            target += 64u + ((i * 13u) % 97u);
        if (target > remaining - min_tail)
            target = remaining - min_tail;
        if (target == 0)
            target = 1;
        state->subpath_lengths[i] = target;
        remaining -= target;
        remaining_paths--;
    }
    if (remaining > 0)
        state->subpath_lengths[count - 1u] += remaining;
    return count;
}



/**
 * Fill one RGBA color from a cool-to-warm gradient.
 *
 * @param t normalized color coordinate
 * @param out output RGBA color
 */
static void _gradient_color(float t, DvzColor out)
{
    ANN(out);
    t = _clamp_float(t, 0.0f, 1.0f);
    float r = 0.0f;
    float g = 0.0f;
    float b = 0.0f;

    if (t < 0.33f)
    {
        float u = t / 0.33f;
        r = 35.0f + 55.0f * u;
        g = 120.0f + 110.0f * u;
        b = 250.0f - 70.0f * u;
    }
    else if (t < 0.66f)
    {
        float u = (t - 0.33f) / 0.33f;
        r = 90.0f + 160.0f * u;
        g = 230.0f - 25.0f * u;
        b = 180.0f - 120.0f * u;
    }
    else
    {
        float u = (t - 0.66f) / 0.34f;
        r = 250.0f - 15.0f * u;
        g = 205.0f - 145.0f * u;
        b = 60.0f + 30.0f * u;
    }

    out[0] = (uint8_t)_clamp_float(r, 0.0f, 255.0f);
    out[1] = (uint8_t)_clamp_float(g, 0.0f, 255.0f);
    out[2] = (uint8_t)_clamp_float(b, 0.0f, 255.0f);
    out[3] = 255;
}



/**
 * Rebuild the active path attribute arrays from the current controls.
 *
 * @param state example state
 * @return effective subpath count
 */
static uint32_t _build_path_data(PathStressState* state)
{
    ANN(state);
    ANN(state->positions);
    ANN(state->colors);
    ANN(state->stroke_widths);

    uint32_t subpath_count = _build_subpaths(state);
    uint32_t offset = 0;
    float lane_span = subpath_count > 1 ? 1.72f / (float)(subpath_count - 1u) : 0.0f;
    float lane_amp = subpath_count > 16 ? 0.35f * lane_span : 0.11f;
    if (lane_amp < 0.006f)
        lane_amp = 0.006f;

    for (uint32_t sp = 0; sp < subpath_count; sp++)
    {
        uint32_t length = state->subpath_lengths[sp];
        float lane_y = subpath_count > 1 ? -0.86f + lane_span * (float)sp : 0.0f;
        float sp_phase = 0.23f * (float)(sp % 23u);

        for (uint32_t j = 0; j < length; j++)
        {
            uint32_t idx = offset + j;
            float denom = length > 1 ? (float)(length - 1u) : 1.0f;
            float u = (float)j / denom;
            float global_t = state->active_count > 1 ?
                                 (float)idx / (float)(state->active_count - 1u) :
                                 0.0f;
            float x = -0.94f + 1.88f * u;
            float y = lane_y;

            if (state->high_frequency_zigzag)
            {
                float sign = (j & 1u) != 0 ? -1.0f : 1.0f;
                y += sign * lane_amp;
                y += 0.35f * lane_amp * sinf(130.0f * u + sp_phase);
            }
            else
            {
                y += lane_amp * sinf(TAU * (3.0f + (float)(sp % 7u)) * u + sp_phase);
                y += 0.42f * lane_amp * sinf(TAU * 17.0f * u + 0.7f * sp_phase);
            }

            if (state->animated_deformation)
            {
                y += 0.52f * lane_amp * sinf(state->phase + TAU * 5.0f * u + sp_phase);
                x += 0.025f * sinf(0.63f * state->phase + TAU * 2.0f * u + sp_phase);
            }

            state->positions[idx][0] = x;
            state->positions[idx][1] = _clamp_float(y, -0.98f, 0.98f);
            state->positions[idx][2] = 0.0f;

            if (state->color_gradient)
            {
                _gradient_color(global_t, state->colors[idx]);
            }
            else
            {
                state->colors[idx][0] = 232;
                state->colors[idx][1] = 236;
                state->colors[idx][2] = 230;
                state->colors[idx][3] = 255;
            }

            float width = _clamp_float(state->global_stroke_width, 0.5f, 32.0f);
            if (state->per_point_width)
            {
                float wave = 0.5f + 0.5f * sinf(TAU * 9.0f * global_t + sp_phase);
                width *= 0.42f + 1.15f * wave;
            }
            state->stroke_widths[idx] = _clamp_float(width, 0.5f, 40.0f);
        }
        offset += length;
    }

    return subpath_count;
}



/**
 * Upload the active arrays to both path visuals.
 *
 * @param state example state
 */
static void _upload_path_data(PathStressState* state)
{
    ANN(state);
    ANN(state->thin_visual);
    ANN(state->stroked_visual);

    uint32_t subpath_count = _build_path_data(state);

    const DvzVisualDataUpdate thin_updates[] = {
        {.attr_name = "position", .data = state->positions, .item_count = state->active_count},
        {.attr_name = "color", .data = state->colors, .item_count = state->active_count},
    };
    if (dvz_visual_set_data_many(state->thin_visual, thin_updates, 2) != 0)
        dvz_fprintf(stderr, "failed to update thin path data\n");

    const DvzVisualDataUpdate stroked_updates[] = {
        {.attr_name = "position", .data = state->positions, .item_count = state->active_count},
        {.attr_name = "color", .data = state->colors, .item_count = state->active_count},
        {
            .attr_name = "stroke_width",
            .data = state->stroke_widths,
            .item_count = state->active_count,
        },
    };
    if (dvz_visual_set_data_many(state->stroked_visual, stroked_updates, 3) != 0)
        dvz_fprintf(stderr, "failed to update stroked path data\n");

    if (dvz_path_set_subpaths(state->stroked_visual, subpath_count, state->subpath_lengths) != 0)
        dvz_fprintf(stderr, "failed to update stroked path subpaths\n");

    dvz_visual_set_visible(state->thin_visual, !state->stroked_path);
    dvz_visual_set_visible(state->stroked_visual, state->stroked_path);
}



/**
 * Apply one vertex-count preset and upload the visual data.
 *
 * @param state example state
 * @param count requested active vertex count
 */
static void _apply_vertex_preset(PathStressState* state, uint32_t count)
{
    ANN(state);
    state->active_count = _clamp_u32(count, PATH_MIN_VERTICES, PATH_MAX_VERTICES);
    _upload_path_data(state);
}



/**
 * Release host-side buffers owned by the example.
 *
 * @param state example state
 */
static void _destroy_state(PathStressState* state)
{
    if (state == NULL)
        return;
    dvz_free(state->positions);
    dvz_free(state->colors);
    dvz_free(state->stroke_widths);
    dvz_free(state->subpath_lengths);
    state->positions = NULL;
    state->colors = NULL;
    state->stroke_widths = NULL;
    state->subpath_lengths = NULL;
}



/*************************************************************************************************/
/*  Callbacks                                                                                    */
/*************************************************************************************************/

/**
 * Build live controls for path stress parameters.
 *
 * @param gui GUI overlay
 * @param win app window
 * @param user_data example state
 */
static void _path_stress_gui(DvzGui* gui, DvzAppWindow* win, void* user_data)
{
    (void)win;
    PathStressState* state = (PathStressState*)user_data;
    if (state == NULL)
        return;

    bool changed = false;
    if (dvz_gui_begin(gui, "Path Stress", NULL, 0))
    {
        int active = (int)state->active_count;
        if (igSliderInt("Active vertices", &active, (int)PATH_MIN_VERTICES, (int)PATH_MAX_VERTICES,
                        "%d", 0))
        {
            state->active_count = (uint32_t)active;
            changed = true;
        }

        if (igButton("512", (ImVec2){0.0f, 0.0f}))
            _apply_vertex_preset(state, 512u);
        igSameLine(0.0f, 6.0f);
        if (igButton("4k", (ImVec2){0.0f, 0.0f}))
            _apply_vertex_preset(state, 4096u);
        igSameLine(0.0f, 6.0f);
        if (igButton("16k", (ImVec2){0.0f, 0.0f}))
            _apply_vertex_preset(state, 16384u);
        igSameLine(0.0f, 6.0f);
        if (igButton("64k", (ImVec2){0.0f, 0.0f}))
            _apply_vertex_preset(state, 65536u);

        const char* const modes[] = {"Single", "Even", "Staggered"};
        int mode = (int)state->subpath_mode;
        if (dvz_gui_combo(gui, "Subpath mode", &mode, modes, 3))
        {
            state->subpath_mode = (PathStressSubpathMode)mode;
            changed = true;
        }

        int subpaths = (int)state->subpath_count;
        if (igSliderInt("Subpaths", &subpaths, 1, (int)PATH_MAX_SUBPATHS, "%d", 0))
        {
            state->subpath_count = (uint32_t)subpaths;
            changed = true;
        }

        changed |= dvz_gui_checkbox(gui, "Stroked path", &state->stroked_path);
        changed |= dvz_gui_checkbox(gui, "Per-point width", &state->per_point_width);
        changed |= dvz_gui_slider_float_format(
            gui, "Stroke width", &state->global_stroke_width, 0.5f, 28.0f, "%.1f px");
        changed |= dvz_gui_checkbox(gui, "Animated deformation", &state->animated_deformation);
        changed |= dvz_gui_checkbox(gui, "Color gradient", &state->color_gradient);
        changed |= dvz_gui_checkbox(gui, "High-frequency zigzag", &state->high_frequency_zigzag);
    }
    dvz_gui_end(gui);

    if (changed)
        _upload_path_data(state);
}



/**
 * Animate path data after each submitted frame.
 *
 * @param win app-window whose frame just completed
 * @param user_data example state
 */
static void _path_stress_frame(DvzAppWindow* win, void* user_data)
{
    PathStressState* state = (PathStressState*)user_data;
    if (state == NULL)
        return;

    state->frame_index++;
    if (!state->animated_deformation)
        return;

    state->phase += 0.055f;
    _upload_path_data(state);
    dvz_app_window_request_frame(win);
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Run the live path visual stress example.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @return process exit status
 */
int main(int argc, char** argv)
{
    int ret = 1;
    DvzScene* scene = NULL;
    DvzApp* app = NULL;
    PathStressState state = {
        .active_count = PATH_DEFAULT_VERTICES,
        .subpath_count = 8u,
        .subpath_mode = PATH_STRESS_SUBPATH_EVEN,
        .stroked_path = true,
        .per_point_width = true,
        .animated_deformation = true,
        .color_gradient = true,
        .high_frequency_zigzag = false,
        .global_stroke_width = 5.0f,
    };

    state.positions = (float(*)[3])dvz_calloc(PATH_MAX_VERTICES, sizeof(*state.positions));
    state.colors = (DvzColor*)dvz_calloc(PATH_MAX_VERTICES, sizeof(*state.colors));
    state.stroke_widths = (float*)dvz_calloc(PATH_MAX_VERTICES, sizeof(*state.stroke_widths));
    state.subpath_lengths =
        (uint32_t*)dvz_calloc(PATH_MAX_SUBPATHS, sizeof(*state.subpath_lengths));
    EXAMPLE_CHECK(
        state.positions != NULL && state.colors != NULL && state.stroke_widths != NULL &&
            state.subpath_lengths != NULL,
        "failed to allocate path stress buffers");

    scene = dvz_scene();
    EXAMPLE_CHECK(scene != NULL, "dvz_scene() failed");

    DvzFigure* figure = dvz_figure(scene, WIDTH, HEIGHT, 0);
    EXAMPLE_CHECK(figure != NULL, "dvz_figure() failed");

    DvzPanel* panel = dvz_panel_full(figure);
    EXAMPLE_CHECK(panel != NULL, "dvz_panel() failed");

    state.thin_visual = dvz_path(scene, 0);
    state.stroked_visual = dvz_path(scene, 0);
    EXAMPLE_CHECK(
        state.thin_visual != NULL && state.stroked_visual != NULL, "dvz_path() failed");

    _upload_path_data(&state);
    EXAMPLE_CHECK(
        dvz_panel_add_visual(panel, state.thin_visual, NULL) == 0 &&
            dvz_panel_add_visual(panel, state.stroked_visual, NULL) == 0,
        "dvz_panel_add_visual() failed");
    dvz_panel_set_background_color(panel, 0.035f, 0.040f, 0.050f, 1.0f);

    app = dvz_app(scene);
    EXAMPLE_CHECK(app != NULL, "dvz_app() failed (no GPU or display?)");

    DvzAppWindow* win =
        dvz_app_window_glfw(app, figure, WIDTH, HEIGHT, "path");
    EXAMPLE_CHECK(win != NULL, "dvz_app_window_glfw() failed (GLFW unavailable?)");

    DvzPanzoom* panzoom = dvz_app_window_panel_panzoom(win, panel, NULL);
    EXAMPLE_CHECK(panzoom != NULL, "failed to create or bind panzoom controller");

    DvzGuiConfig gui_config = dvz_gui_config();
    DvzGui* gui = dvz_app_window_gui(win, &gui_config);
    EXAMPLE_CHECK(gui != NULL, "dvz_app_window_gui() failed");
    dvz_app_window_set_gui_callback(win, _path_stress_gui, &state);
    dvz_app_window_set_frame_callback(win, _path_stress_frame, &state);

    dvz_app_run(app, example_frame_count(argc, argv));
    ret = 0;

cleanup:
    if (app != NULL)
        dvz_app_destroy(app);
    if (scene != NULL)
        dvz_scene_destroy(scene);
    _destroy_state(&state);
    return ret;
}
