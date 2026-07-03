/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* pixel - deterministic retained pixel visual baseline.
 *
 * Scenario: visual.pixel
 * Style: visuals, graphite_cyan, 1280x720 window target
 *
 * Build:  just example-c visuals/pixel
 * Run:    ./build/examples/c/visuals/pixel --live
 * Smoke:  ./build/examples/c/visuals/pixel --png
 */



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <math.h>
#include <stdbool.h>
#include <stdint.h>

#include "_alloc.h"
#include "_assertions.h"
#include "datoviz/scene.h"
#include "example_style.h"
#include "runner/scenario_runner.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH  EXAMPLE_WINDOW_WIDTH
#define HEIGHT EXAMPLE_WINDOW_HEIGHT
#define GRID_WIDTH  72u
#define GRID_HEIGHT 54u
#define PIXEL_COUNT (GRID_WIDTH * GRID_HEIGHT)

static const float TAU = 6.28318530718f;



/*************************************************************************************************/
/*  Forward declarations                                                                         */
/*************************************************************************************************/

DvzScenarioSpec dvz_visual_pixel_scenario(void);



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct PixelVisualState
{
    vec3* positions;
    float* values;
    float* sizes;
} PixelVisualState;



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Return one deterministic scalar sample for the pixel grid.
 *
 * @param u normalized grid x coordinate
 * @param v normalized grid y coordinate
 * @return normalized scalar value
 */
static float _sample_value(float u, float v)
{
    const float ridge = 0.5f + 0.5f * sinf(TAU * (1.7f * u + 0.35f * v));
    const float wave = 0.5f + 0.5f * cosf(TAU * (0.6f * u - 1.9f * v));
    const float dx = u - 0.34f;
    const float dy = v - 0.64f;
    const float blob = expf(-(dx * dx + 1.6f * dy * dy) / 0.012f);
    const float value = 0.08f + 0.36f * u + 0.18f * v + 0.16f * ridge + 0.10f * wave +
                        0.28f * blob;
    return fminf(1.0f, fmaxf(0.0f, value));
}



/**
 * Fill the deterministic pixel grid.
 *
 * @param positions output pixel positions
 * @param values output scalar values
 * @param sizes output pixel sprite sizes in pixels
 */
static void _fill_pixels(
    vec3 positions[PIXEL_COUNT], float values[PIXEL_COUNT], float sizes[PIXEL_COUNT])
{
    ANN(positions);
    ANN(values);
    ANN(sizes);

    const float step_x = 1.82f / (float)(GRID_WIDTH - 1u);
    const float step_y = 1.36f / (float)(GRID_HEIGHT - 1u);

    for (uint32_t y = 0; y < GRID_HEIGHT; y++)
    {
        for (uint32_t x = 0; x < GRID_WIDTH; x++)
        {
            const uint32_t i = y * GRID_WIDTH + x;
            const float u = (float)x / (float)(GRID_WIDTH - 1u);
            const float v = (float)y / (float)(GRID_HEIGHT - 1u);
            const float value = _sample_value(u, v);

            positions[i][0] = -0.91f + step_x * (float)x;
            positions[i][1] = -0.68f + step_y * (float)y;
            positions[i][2] = 0.0f;

            values[i] = value;
            sizes[i] = 1.0f + 4.0f * value;
        }
    }
}



/**
 * Free the retained pixel buffers.
 *
 * @param state pixel visual state
 */
static void _free_state(PixelVisualState* state)
{
    if (state == NULL)
        return;
    dvz_free(state->sizes);
    dvz_free(state->values);
    dvz_free(state->positions);
    dvz_free(state);
}



/*************************************************************************************************/
/*  Scenario callbacks                                                                           */
/*************************************************************************************************/

/**
 * Initialize the retained pixel visual scenario.
 *
 * @param ctx scenario context
 * @param out_user scenario state output
 * @return true on success
 */
static bool _scenario_init(DvzScenarioContext* ctx, void** out_user)
{
    if (ctx == NULL)
        return false;
    if (out_user != NULL)
        *out_user = NULL;

    PixelVisualState* state = (PixelVisualState*)dvz_calloc(1, sizeof(*state));
    if (state == NULL)
        return false;

    state->positions = (vec3*)dvz_calloc(PIXEL_COUNT, sizeof(*state->positions));
    state->values = (float*)dvz_calloc(PIXEL_COUNT, sizeof(*state->values));
    state->sizes = (float*)dvz_calloc(PIXEL_COUNT, sizeof(*state->sizes));
    if (state->positions == NULL || state->values == NULL || state->sizes == NULL)
        goto error;

    _fill_pixels(state->positions, state->values, state->sizes);

    ctx->figure = dvz_figure(ctx->scene, ctx->width, ctx->height, 0);
    if (ctx->figure == NULL)
        goto error;

    DvzPanel* panel = dvz_panel_full(ctx->figure);
    if (panel == NULL)
        goto error;
    example_graphite_cyan_set_panel_background(panel);

    DvzVisual* visual = dvz_pixel(ctx->scene, 0);
    if (visual == NULL)
        goto error;

    int rc = dvz_visual_set_attr_format(visual, "color", DVZ_VISUAL_ATTR_FORMAT_SCALAR_F32);
    if (rc != 0)
        goto error;

    DvzScale* color_scale = example_graphite_cyan_color_scale(ctx->scene, 0.0, 1.0);
    if (color_scale == NULL)
        goto error;

    rc = dvz_visual_set_scale(visual, "color", color_scale);
    if (rc != 0)
        goto error;

    DvzVisualDataUpdate updates[] = {
        {.attr_name = "position", .data = state->positions, .item_count = PIXEL_COUNT},
        {.attr_name = "color", .data = state->values, .item_count = PIXEL_COUNT},
        {.attr_name = "pixel_size_px", .data = state->sizes, .item_count = PIXEL_COUNT},
    };
    rc = dvz_visual_set_data_many(visual, updates, 3);
    if (rc != 0)
        goto error;

    rc = dvz_visual_set_depth_test(visual, false);
    if (rc != 0)
        goto error;

    rc = dvz_panel_add_visual(panel, visual, NULL);
    if (rc != 0)
        goto error;

    DvzPanzoom* panzoom = dvz_scenario_panzoom(ctx, panel, NULL, DVZ_DIM_MASK_XY);
    if (panzoom == NULL)
        goto error;

    if (out_user != NULL)
        *out_user = state;
    return true;

error:
    _free_state(state);
    return false;
}



/**
 * Destroy the retained pixel visual scenario state.
 *
 * @param ctx scenario context
 * @param user scenario state
 */
static void _scenario_destroy(DvzScenarioContext* ctx, void* user)
{
    (void)ctx;
    _free_state((PixelVisualState*)user);
}



/**
 * Return the pixel visual scenario specification.
 *
 * @return scenario specification
 */
DvzScenarioSpec dvz_visual_pixel_scenario(void)
{
    return (DvzScenarioSpec){
        .id = "visual_pixel",
        .title = "Pixel",
        .width = WIDTH,
        .height = HEIGHT,
        .fps = 60.0,
        .init = _scenario_init,
        .destroy = _scenario_destroy,
    };
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Run the retained pixel visual example through the native scenario runner.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @return process exit code
 */
#ifndef DVZ_EXAMPLE_NO_MAIN
int main(int argc, char** argv)
{
    DvzScenarioSpec spec = dvz_visual_pixel_scenario();
    return dvz_scenario_run_native_cli(&spec, argc, argv) == 0 ? 0 : 1;
}
#endif
