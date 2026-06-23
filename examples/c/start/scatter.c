/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* scatter - quickstart scatter plot: 10 000 random colored points with pan/zoom.
 *
 * Scenario: start.scatter
 * Style: start, graphite_cyan, 1600x1200 capture target
 *
 * Build:  just example-c start/scatter
 * Run:    ./build/examples/c/start/scatter --live
 * Smoke:  ./build/examples/c/start/scatter --png
 */



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "_alloc.h"
#include "_assertions.h"
#include "datoviz/scene.h"
#include "example_style.h"
#include "runner/scenario_runner.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH       1600u
#define HEIGHT      1200u
#define POINT_COUNT 10000u
#define SEED        12345u



/*************************************************************************************************/
/*  Forward declarations                                                                         */
/*************************************************************************************************/

DvzScenarioSpec dvz_start_scatter_scenario(void);



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct ScatterState
{
    vec3* positions;
    DvzColor* colors;
    float* diameters;
} ScatterState;



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

static float _randf(void) { return (float)rand() / (float)RAND_MAX; }

static void _fill_scatter(
    vec3 positions[POINT_COUNT], DvzColor colors[POINT_COUNT], float diameters[POINT_COUNT])
{
    ANN(positions);
    ANN(colors);
    ANN(diameters);

    srand(SEED);
    for (uint32_t i = 0; i < POINT_COUNT; i++)
    {
        positions[i][0] = 2.0f * _randf() - 1.0f;
        positions[i][1] = 2.0f * _randf() - 1.0f;
        positions[i][2] = 0.0f;

        colors[i].r = (uint8_t)(_randf() * 255);
        colors[i].g = (uint8_t)(_randf() * 255);
        colors[i].b = (uint8_t)(_randf() * 255);
        colors[i].a = 200;

        diameters[i] = 4.0f + 8.0f * _randf();
    }
}

static void _free_state(ScatterState* state)
{
    if (state == NULL)
        return;
    dvz_free(state->diameters);
    dvz_free(state->colors);
    dvz_free(state->positions);
    dvz_free(state);
}



/*************************************************************************************************/
/*  Scenario callbacks                                                                           */
/*************************************************************************************************/

static bool _scenario_init(DvzScenarioContext* ctx, void** out_user)
{
    if (ctx == NULL)
        return false;
    if (out_user != NULL)
        *out_user = NULL;

    ScatterState* state = (ScatterState*)dvz_calloc(1, sizeof(*state));
    if (state == NULL)
        return false;

    state->positions = (vec3*)dvz_calloc(POINT_COUNT, sizeof(*state->positions));
    state->colors    = (DvzColor*)dvz_calloc(POINT_COUNT, sizeof(*state->colors));
    state->diameters = (float*)dvz_calloc(POINT_COUNT, sizeof(*state->diameters));
    if (state->positions == NULL || state->colors == NULL || state->diameters == NULL)
        goto error;

    _fill_scatter(state->positions, state->colors, state->diameters);

    ctx->figure = dvz_figure(ctx->scene, ctx->width, ctx->height, 0);
    if (ctx->figure == NULL)
        goto error;

    DvzPanel* panel = dvz_panel_full(ctx->figure);
    if (panel == NULL)
        goto error;
    example_graphite_cyan_set_panel_background(panel);

    DvzVisual* point = dvz_point(ctx->scene, 0);
    if (point == NULL)
        goto error;

    DvzVisualDataUpdate updates[] = {
        {.attr_name = "position", .data = state->positions, .item_count = POINT_COUNT},
        {.attr_name = "color",    .data = state->colors,    .item_count = POINT_COUNT},
        {.attr_name = "diameter_px", .data = state->diameters, .item_count = POINT_COUNT},
    };
    if (dvz_visual_set_data_many(point, updates, 3) != 0)
        goto error;

    DvzPointStyleDesc style = dvz_point_style_desc();
    style.aspect       = DVZ_SHAPE_ASPECT_FILLED;
    style.stroke_width_px = 0.0f;
    if (dvz_point_set_style(point, &style) != 0)
        goto error;
    if (dvz_visual_set_depth_test(point, false) != 0)
        goto error;
    if (dvz_visual_set_alpha_mode(point, DVZ_ALPHA_BLENDED) != 0)
        goto error;
    if (dvz_panel_add_visual(panel, point, NULL) != 0)
        goto error;

    if (dvz_scenario_panzoom(ctx, panel, NULL, DVZ_DIM_MASK_XY) == NULL)
        goto error;

    if (out_user != NULL)
        *out_user = state;
    return true;

error:
    _free_state(state);
    return false;
}

static void _scenario_destroy(DvzScenarioContext* ctx, void* user)
{
    (void)ctx;
    _free_state((ScatterState*)user);
}

DvzScenarioSpec dvz_start_scatter_scenario(void)
{
    return (DvzScenarioSpec){
        .id      = "start_scatter",
        .title   = "start_scatter",
        .width   = WIDTH,
        .height  = HEIGHT,
        .fps     = 60.0,
        .init    = _scenario_init,
        .destroy = _scenario_destroy,
    };
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

#ifndef DVZ_EXAMPLE_NO_MAIN
int main(int argc, char** argv)
{
    DvzScenarioSpec spec = dvz_start_scatter_scenario();
    return dvz_scenario_run_native_cli(&spec, argc, argv) == 0 ? 0 : 1;
}
#endif
