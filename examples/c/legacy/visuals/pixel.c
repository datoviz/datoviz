/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* pixel - live pixel-visual stress workbench.
 *
 * Build:  just build
 * Run:    just example-c visuals/pixel
 * Smoke:  ./build/examples/c/visuals/pixel 120
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

#define WIDTH  1100u
#define HEIGHT 760u

#define MAX_PIXELS 262144u

static const float TAU = 6.28318530718f;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct PixelState
{
    DvzVisual* visual;
    DvzView* win;
    vec3* positions;
    DvzColor* colors;
    float* sizes;
    uint32_t max_count;
    uint32_t active_count;
    float count_value;
    float pixel_size_px;
    float alpha;
    float phase;
    bool animate;
    bool depth_variation;
} PixelState;



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Fill deterministic pixel buffers for the active count.
 *
 * @param state pixel workbench state
 * @param phase animation phase in radians
 */
static void _fill_pixels(PixelState* state, float phase)
{
    ANN(state);
    ANN(state->positions);
    ANN(state->colors);
    ANN(state->sizes);

    const uint32_t count = state->active_count;
    const uint32_t side = (uint32_t)ceilf(sqrtf((float)count));
    const float inv_side = side > 1 ? 1.0f / (float)(side - 1) : 1.0f;

    for (uint32_t i = 0; i < count; i++)
    {
        const uint32_t x = i % side;
        const uint32_t y = i / side;
        const float u = (float)x * inv_side;
        const float v = (float)y * inv_side;
        const float px = 2.0f * u - 1.0f;
        const float py = 2.0f * v - 1.0f;
        const float r = sqrtf(px * px + py * py);
        const float angle = atan2f(py, px);
        const float spiral = angle + 0.42f * sinf(phase * 0.45f + 5.5f * r);
        const float dome = fmaxf(0.0f, 1.0f - 0.62f * r * r);
        const float ridge = 0.5f + 0.5f * sinf(phase + 17.0f * r - 5.0f * angle);
        const float wave = sinf(phase + 12.0f * u + 16.0f * v);
        const float z = state->depth_variation ? 0.72f * dome + 0.20f * wave : 0.0f;

        state->positions[i][0] = 1.36f * r * cosf(spiral);
        state->positions[i][1] = 1.02f * r * sinf(spiral);
        state->positions[i][2] = z - 0.34f;
        state->sizes[i] = state->pixel_size_px * (0.62f + 0.58f * ridge);
        state->colors[i] = dvz_color_rgba(
            (uint8_t)(70u + (uint32_t)(145.0f * ridge)),
            (uint8_t)(58u + (uint32_t)(150.0f * dome)),
            (uint8_t)(105u + (uint32_t)(118.0f * (1.0f - 0.45f * u + 0.25f * v))),
            (uint8_t)(255.0f * state->alpha));
    }
}



/**
 * Upload the active pixel arrays.
 *
 * @param state pixel workbench state
 * @return true if the upload succeeded
 */
static bool _upload_pixels(PixelState* state)
{
    ANN(state);
    ANN(state->visual);

    DvzVisualDataUpdate updates[] = {
        {.attr_name = "position", .data = state->positions, .item_count = state->active_count},
        {.attr_name = "color", .data = state->colors, .item_count = state->active_count},
        {.attr_name = "pixel_size_px", .data = state->sizes, .item_count = state->active_count},
    };
    if (dvz_visual_set_data_many(state->visual, updates, 3) != 0)
        return false;
    return dvz_visual_set_alpha_mode(
               state->visual, state->alpha < 0.999f ? DVZ_ALPHA_BLENDED : DVZ_ALPHA_OPAQUE) == 0;
}



/**
 * Render the pixel controls.
 *
 * @param gui GUI context
 * @param win view
 * @param user_data pixel workbench state
 */
static void _gui_callback(DvzGui* gui, DvzView* win, void* user_data)
{
    (void)win;
    PixelState* state = (PixelState*)user_data;
    ANN(state);

    bool changed = false;
    if (dvz_gui_begin(gui, "Pixel", NULL, 0))
    {
        changed |= dvz_gui_slider_float(gui, "Count", &state->count_value, 1024.0f, MAX_PIXELS);
        changed |= dvz_gui_slider_float(gui, "Pixel size", &state->pixel_size_px, 1.0f, 12.0f);
        changed |= dvz_gui_slider_float(gui, "Alpha", &state->alpha, 0.02f, 1.0f);
        changed |= dvz_gui_checkbox(gui, "Depth variation", &state->depth_variation);
        (void)dvz_gui_checkbox(gui, "Animate", &state->animate);
    }
    dvz_gui_end(gui);

    if (changed)
    {
        state->active_count = (uint32_t)state->count_value;
        if (state->active_count < 1)
            state->active_count = 1;
        if (state->active_count > state->max_count)
            state->active_count = state->max_count;
        _fill_pixels(state, 0.0f);
        (void)_upload_pixels(state);
        dvz_view_request_frame(state->win);
    }
}



/**
 * Update animated pixel data before each frame.
 *
 * @param win view
 * @param user_data pixel workbench state
 */
static void _frame_callback(DvzView* win, void* user_data)
{
    PixelState* state = (PixelState*)user_data;
    if (state == NULL || !state->animate)
        return;

    state->phase += 0.035f;
    _fill_pixels(state, state->phase);
    (void)_upload_pixels(state);
    dvz_view_request_frame(win);
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Run the pixel visual stress workbench.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @return process exit code
 */
int main(int argc, char** argv)
{
    int ret = 1;
    DvzScene* scene = NULL;
    DvzApp* app = NULL;
    PixelState state = {
        .max_count = MAX_PIXELS,
        .active_count = 65536u,
        .count_value = 65536.0f,
        .pixel_size_px = 3.4f,
        .alpha = 1.0f,
        .animate = true,
        .depth_variation = true,
    };
    state.positions = dvz_calloc(MAX_PIXELS, sizeof(*state.positions));
    state.colors = dvz_calloc(MAX_PIXELS, sizeof(*state.colors));
    state.sizes = dvz_calloc(MAX_PIXELS, sizeof(*state.sizes));
    EXAMPLE_CHECK(
        state.positions != NULL && state.colors != NULL && state.sizes != NULL,
        "pixel buffer allocation failed");

    scene = dvz_scene();
    EXAMPLE_CHECK(scene != NULL, "dvz_scene() failed");

    DvzFigure* figure = dvz_figure(scene, WIDTH, HEIGHT, 0);
    DvzPanel* panel = figure != NULL ? dvz_panel_full(figure) : NULL;
    state.visual = panel != NULL ? dvz_pixel(scene, 0) : NULL;
    EXAMPLE_CHECK(
        figure != NULL && panel != NULL && state.visual != NULL, "pixel scene setup failed");

    DvzCameraDesc camera_desc = dvz_camera_desc();
    camera_desc.eye[2] = 3.6f;
    camera_desc.near_clip = 0.1f;
    camera_desc.far_clip = 100.0f;
    bool ok = dvz_panel_set_camera(panel, &camera_desc);
    EXAMPLE_CHECK(ok, "dvz_panel_set_camera() failed");

    dvz_panel_set_background_color(panel, dvz_color_from_unit(0.030f, 0.036f, 0.045f, 1.0f));
    DvzDepthCueDesc cue = {DVZ_STRUCT_INIT_FIELDS(DvzDepthCueDesc),
        .mode = DVZ_DEPTH_CUE_FADE_TO_BACKGROUND,
        .metric = DVZ_DEPTH_CUE_METRIC_EYE_DISTANCE,
        .falloff = DVZ_DEPTH_CUE_FALLOFF_LINEAR,
        .near_depth = 2.5f,
        .far_depth = 4.8f,
        .strength = 0.58f,
        .density = 2.6f,
        .background_color = {0.030f, 0.036f, 0.045f, 1.0f},
    };
    _fill_pixels(&state, 0.0f);
    ok = _upload_pixels(&state);
    EXAMPLE_CHECK(ok, "pixel data upload failed");

    int rc = dvz_visual_set_depth_cue(state.visual, &cue);
    EXAMPLE_CHECK(rc == 0, "dvz_visual_set_depth_cue() failed");

    rc = dvz_panel_add_visual(panel, state.visual, NULL);
    EXAMPLE_CHECK(rc == 0, "dvz_panel_add_visual() failed");

    app = dvz_app(scene);
    EXAMPLE_CHECK(app != NULL, "dvz_app() failed");

    state.win = dvz_view_glfw(app, figure, WIDTH, HEIGHT, "pixel");
    EXAMPLE_CHECK(state.win != NULL, "dvz_view_glfw() failed");

    DvzArcball* arcball = dvz_view_arcball(state.win, panel, NULL);
    EXAMPLE_CHECK(arcball != NULL, "failed to create or bind arcball controller");
    dvz_arcball_set(arcball, (vec3){+0.48f, -0.12f, +0.24f});
    DvzGui* gui = dvz_view_gui(state.win, NULL);
    if (gui != NULL)
        dvz_view_set_gui_callback(state.win, _gui_callback, &state);
    dvz_view_set_frame_callback(state.win, _frame_callback, &state);

    dvz_app_run(app, example_frame_count(argc, argv));
    ret = 0;

cleanup:
    if (app != NULL)
        dvz_app_destroy(app);
    if (scene != NULL)
        dvz_scene_destroy(scene);
    dvz_free(state.sizes);
    dvz_free(state.colors);
    dvz_free(state.positions);
    return ret;
}
