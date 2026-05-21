/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* primitive - live primitive-visual stress workbench.
 *
 * Build:  just build
 * Run:    just example-c visuals/primitive
 * Smoke:  ./build/examples/c/visuals/primitive 120
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

#define MAX_TRIANGLES 65536u
#define VERTICES_PER_TRIANGLE 3u

static const float TAU = 6.28318530718f;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct PrimitiveState
{
    DvzVisual* visual;
    DvzAppWindow* win;
    float (*positions)[3];
    DvzColor* colors;
    uint32_t max_triangles;
    uint32_t triangle_count;
    float triangle_value;
    float scale;
    float alpha;
    float phase;
    bool animate;
    bool blended;
} PrimitiveState;



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
 * Fill deterministic triangle-list buffers.
 *
 * @param state primitive workbench state
 * @param phase animation phase in radians
 */
static void _fill_triangles(PrimitiveState* state, float phase)
{
    ANN(state);
    ANN(state->positions);
    ANN(state->colors);

    const uint32_t count = state->triangle_count;
    const uint32_t side = (uint32_t)ceilf(sqrtf((float)count));
    const float inv_side = side > 1 ? 1.0f / (float)(side - 1) : 1.0f;
    const float radius = state->scale / (float)side;

    for (uint32_t i = 0; i < count; i++)
    {
        const uint32_t x = i % side;
        const uint32_t y = i / side;
        const float u = (float)x * inv_side;
        const float v = (float)y * inv_side;
        const float cx = 2.0f * u - 1.0f;
        const float cy = 2.0f * v - 1.0f;
        const float angle = phase + TAU * (0.11f * (float)x + 0.07f * (float)y);
        const uint32_t base = VERTICES_PER_TRIANGLE * i;

        for (uint32_t k = 0; k < VERTICES_PER_TRIANGLE; k++)
        {
            const float a = angle + TAU * (float)k / 3.0f;
            state->positions[base + k][0] = cx + radius * cosf(a);
            state->positions[base + k][1] = cy + radius * sinf(a);
            state->positions[base + k][2] = 0.25f * sinf(phase + 0.017f * (float)i);
            state->colors[base + k][0] = (uint8_t)(40u + (uint32_t)(170.0f * u));
            state->colors[base + k][1] = (uint8_t)(48u + (uint32_t)(180.0f * v));
            state->colors[base + k][2] = (uint8_t)(220u - (uint32_t)(120.0f * u));
            state->colors[base + k][3] = (uint8_t)(255.0f * state->alpha);
        }
    }
}



/**
 * Upload the active primitive arrays.
 *
 * @param state primitive workbench state
 * @return true if the upload succeeded
 */
static bool _upload_triangles(PrimitiveState* state)
{
    ANN(state);
    ANN(state->visual);

    const uint32_t vertex_count = VERTICES_PER_TRIANGLE * state->triangle_count;
    DvzVisualDataUpdate updates[2] = {
        {.attr_name = "position", .data = state->positions, .item_count = vertex_count},
        {.attr_name = "color", .data = state->colors, .item_count = vertex_count},
    };
    if (dvz_visual_set_data_many(state->visual, updates, 2) != 0)
        return false;

    DvzAlphaMode alpha_mode =
        state->blended || state->alpha < 0.999f ? DVZ_ALPHA_BLENDED : DVZ_ALPHA_OPAQUE;
    return dvz_visual_set_alpha_mode(state->visual, alpha_mode) == 0;
}



/**
 * Render primitive visual controls.
 *
 * @param gui GUI context
 * @param win app window
 * @param user_data primitive workbench state
 */
static void _gui_callback(DvzGui* gui, DvzAppWindow* win, void* user_data)
{
    (void)win;
    PrimitiveState* state = (PrimitiveState*)user_data;
    ANN(state);

    bool changed = false;
    if (dvz_gui_begin(gui, "Primitive", NULL, 0))
    {
        changed |=
            dvz_gui_slider_float(gui, "Triangles", &state->triangle_value, 256.0f, MAX_TRIANGLES);
        changed |= dvz_gui_slider_float(gui, "Scale", &state->scale, 0.5f, 2.5f);
        changed |= dvz_gui_slider_float(gui, "Alpha", &state->alpha, 0.02f, 1.0f);
        changed |= dvz_gui_checkbox(gui, "Blended", &state->blended);
        (void)dvz_gui_checkbox(gui, "Animate", &state->animate);
    }
    dvz_gui_end(gui);

    if (changed)
    {
        state->triangle_count = (uint32_t)state->triangle_value;
        if (state->triangle_count < 1)
            state->triangle_count = 1;
        if (state->triangle_count > state->max_triangles)
            state->triangle_count = state->max_triangles;
        _fill_triangles(state, 0.0f);
        (void)_upload_triangles(state);
        dvz_app_window_request_frame(state->win);
    }
}



/**
 * Update animated primitive data before each frame.
 *
 * @param win app window
 * @param user_data primitive workbench state
 */
static void _frame_callback(DvzAppWindow* win, void* user_data)
{
    PrimitiveState* state = (PrimitiveState*)user_data;
    if (state == NULL || !state->animate)
        return;

    state->phase += 0.025f;
    _fill_triangles(state, state->phase);
    (void)_upload_triangles(state);
    dvz_app_window_request_frame(win);
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Run the primitive visual stress workbench.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @return process exit code
 */
int main(int argc, char** argv)
{
    PrimitiveState state = {
        .max_triangles = MAX_TRIANGLES,
        .triangle_count = 8192u,
        .triangle_value = 8192.0f,
        .scale = 1.0f,
        .alpha = 0.85f,
        .animate = true,
    };
    state.positions = dvz_calloc(MAX_TRIANGLES * VERTICES_PER_TRIANGLE, sizeof(*state.positions));
    state.colors = dvz_calloc(MAX_TRIANGLES * VERTICES_PER_TRIANGLE, sizeof(*state.colors));
    if (state.positions == NULL || state.colors == NULL)
    {
        dvz_fprintf(stderr, "primitive buffer allocation failed\n");
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
    state.visual =
        panel != NULL ? dvz_primitive(scene, DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0) : NULL;
    if (figure == NULL || panel == NULL || state.visual == NULL)
    {
        dvz_fprintf(stderr, "primitive scene setup failed\n");
        dvz_scene_destroy(scene);
        return 1;
    }

    dvz_panel_set_background_color(panel, 0.040f, 0.043f, 0.052f, 1.0f);
    _fill_triangles(&state, 0.0f);
    if (!_upload_triangles(&state) || dvz_panel_add_visual(panel, state.visual, NULL) != 0)
    {
        dvz_fprintf(stderr, "primitive visual setup failed\n");
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
    state.win = dvz_app_window_glfw(app, figure, WIDTH, HEIGHT, "primitive");
    if (state.win == NULL)
    {
        dvz_fprintf(stderr, "dvz_app_window_glfw() failed\n");
        dvz_app_destroy(app);
        dvz_scene_destroy(scene);
        return 1;
    }
    DvzController* panzoom_controller = dvz_panzoom(scene, NULL);
    if (panzoom_controller == NULL ||
        dvz_panel_bind_controller(panel, panzoom_controller, DVZ_DIM_MASK_XY) != 0)
    {
        dvz_fprintf(stderr, "failed to create or bind panzoom controller\n");
        dvz_app_destroy(app);
        dvz_scene_destroy(scene);
        return 1;
    }
    dvz_panel_connect_input(panel, dvz_app_window_input(state.win));
    DvzGui* gui = dvz_app_window_gui(state.win, NULL);
    if (gui != NULL)
        dvz_app_window_set_gui_callback(state.win, _gui_callback, &state);
    dvz_app_window_set_frame_callback(state.win, _frame_callback, &state);

    dvz_app_run(app, _frame_count(argc, argv));

    dvz_app_destroy(app);
    dvz_scene_destroy(scene);
    dvz_free(state.colors);
    dvz_free(state.positions);
    return 0;
}
