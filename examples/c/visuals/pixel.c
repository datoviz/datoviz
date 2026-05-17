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
    DvzAppWindow* win;
    float (*positions)[3];
    DvzColor* colors;
    float* sizes;
    uint32_t max_count;
    uint32_t active_count;
    float count_value;
    float pixel_size;
    float alpha;
    float phase;
    bool animate;
    bool depth_variation;
} PixelState;



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Parse a bounded frame count from the first command-line argument.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @return frame count, or zero for an interactive unbounded run
 */
static uint32_t _frame_count(int argc, char** argv)
{
    if (argc < 2 || argv == NULL || argv[1] == NULL)
        return 0;

    char* end = NULL;
    unsigned long value = strtoul(argv[1], &end, 10);
    if (end == argv[1])
        return 0;
    if (value > UINT32_MAX)
        return UINT32_MAX;
    return (uint32_t)value;
}



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
        const float wave = sinf(phase + 18.0f * u + 9.0f * v);

        state->positions[i][0] = 2.0f * u - 1.0f;
        state->positions[i][1] = 2.0f * v - 1.0f;
        state->positions[i][2] = state->depth_variation ? 0.35f * wave : 0.0f;
        state->sizes[i] = state->pixel_size * (0.7f + 0.3f * (0.5f + 0.5f * wave));
        state->colors[i][0] = (uint8_t)(32u + (uint32_t)(190.0f * u));
        state->colors[i][1] = (uint8_t)(48u + (uint32_t)(180.0f * v));
        state->colors[i][2] = (uint8_t)(220u - (uint32_t)(120.0f * u));
        state->colors[i][3] = (uint8_t)(255.0f * state->alpha);
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

    DvzVisualDataUpdate updates[3] = {
        {.attr_name = "position", .data = state->positions, .item_count = state->active_count},
        {.attr_name = "color", .data = state->colors, .item_count = state->active_count},
        {.attr_name = "pixel_size", .data = state->sizes, .item_count = state->active_count},
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
 * @param win app window
 * @param user_data pixel workbench state
 */
static void _gui_callback(DvzGui* gui, DvzAppWindow* win, void* user_data)
{
    (void)win;
    PixelState* state = (PixelState*)user_data;
    ANN(state);

    bool changed = false;
    if (dvz_gui_begin(gui, "Pixel", NULL, 0))
    {
        changed |= dvz_gui_slider_float(gui, "Count", &state->count_value, 1024.0f, MAX_PIXELS);
        changed |= dvz_gui_slider_float(gui, "Pixel size", &state->pixel_size, 1.0f, 12.0f);
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
        dvz_app_window_request_frame(state->win);
    }
}



/**
 * Update animated pixel data before each frame.
 *
 * @param win app window
 * @param user_data pixel workbench state
 */
static void _frame_callback(DvzAppWindow* win, void* user_data)
{
    PixelState* state = (PixelState*)user_data;
    if (state == NULL || !state->animate)
        return;

    state->phase += 0.035f;
    _fill_pixels(state, state->phase);
    (void)_upload_pixels(state);
    dvz_app_window_request_frame(win);
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
    PixelState state = {
        .max_count = MAX_PIXELS,
        .active_count = 65536u,
        .count_value = 65536.0f,
        .pixel_size = 3.0f,
        .alpha = 0.95f,
        .animate = true,
    };
    state.positions = dvz_calloc(MAX_PIXELS, sizeof(*state.positions));
    state.colors = dvz_calloc(MAX_PIXELS, sizeof(*state.colors));
    state.sizes = dvz_calloc(MAX_PIXELS, sizeof(*state.sizes));
    if (state.positions == NULL || state.colors == NULL || state.sizes == NULL)
    {
        dvz_fprintf(stderr, "pixel buffer allocation failed\n");
        return 1;
    }

    DvzScene* scene = dvz_scene();
    if (scene == NULL)
    {
        dvz_fprintf(stderr, "dvz_scene() failed\n");
        return 1;
    }
    DvzFigure* figure = dvz_figure(scene, WIDTH, HEIGHT, 0);
    DvzPanel* panel = figure != NULL ? dvz_panel(figure, (DvzPanelDesc){0, 0, 1, 1}) : NULL;
    state.visual = panel != NULL ? dvz_pixel(scene, 0) : NULL;
    if (figure == NULL || panel == NULL || state.visual == NULL)
    {
        dvz_fprintf(stderr, "pixel scene setup failed\n");
        dvz_scene_destroy(scene);
        return 1;
    }

    dvz_panel_set_background_color(panel, 0.030f, 0.036f, 0.045f, 1.0f);
    _fill_pixels(&state, 0.0f);
    if (!_upload_pixels(&state) || dvz_panel_add_visual(panel, state.visual, NULL) != 0)
    {
        dvz_fprintf(stderr, "pixel visual setup failed\n");
        dvz_scene_destroy(scene);
        return 1;
    }

    DvzApp* app = dvz_app(scene);
    if (app == NULL)
    {
        dvz_fprintf(stderr, "dvz_app() failed\n");
        dvz_scene_destroy(scene);
        return 1;
    }
    state.win = dvz_app_window_glfw(app, figure, WIDTH, HEIGHT, "pixel");
    if (state.win == NULL)
    {
        dvz_fprintf(stderr, "dvz_app_window_glfw() failed\n");
        dvz_app_destroy(app);
        dvz_scene_destroy(scene);
        return 1;
    }
    dvz_panel_set_panzoom(panel, dvz_app_window_input(state.win), 0);
    DvzGui* gui = dvz_app_window_gui(state.win, NULL);
    if (gui != NULL)
        dvz_app_window_set_gui_callback(state.win, _gui_callback, &state);
    dvz_app_window_set_frame_callback(state.win, _frame_callback, &state);

    dvz_app_run(app, _frame_count(argc, argv));

    dvz_app_destroy(app);
    dvz_scene_destroy(scene);
    dvz_free(state.sizes);
    dvz_free(state.colors);
    dvz_free(state.positions);
    return 0;
}
