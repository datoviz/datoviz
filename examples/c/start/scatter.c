/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* scatter - This example renders a quickstart scatter plot with 10 000 random colored points.
 *
 * Scenario: start_scatter
 * Style: start, graphite_cyan, 1280x720 window target
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "_alloc.h"
#include "_assertions.h"
#include "datoviz/scene.h"
#include "example_random.h"
#include "example_style.h"
#include "runner/scenario_runner.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH                 EXAMPLE_WINDOW_WIDTH
#define HEIGHT                EXAMPLE_WINDOW_HEIGHT
#define DEFAULT_POINT_COUNT   10000u
#define MAX_POINT_COUNT       10000000u
#define SEED                  12345u
#define PANZOOM_PERIOD_FRAMES 240u
#define PANZOOM_WORKLOAD      "panzoom-v1"



/*************************************************************************************************/
/*  Forward declarations                                                                         */
/*************************************************************************************************/

DvzScenarioSpec dvz_start_scatter_scenario(void);



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

typedef struct ScatterState
{
    DvzPanzoom* panzoom;
    bool panzoom_benchmark;
} ScatterState;


/**
 * Return whether the deterministic pan/zoom benchmark workload is requested.
 *
 * @return true when the pan/zoom frame callback is required
 */
static bool _panzoom_benchmark_requested(void)
{
    const char* workload = getenv("DVZ_SCATTER_BENCHMARK");
    return workload != NULL && strcmp(workload, PANZOOM_WORKLOAD) == 0;
}



static void
_fill_scatter(uint32_t point_count, vec3* positions, DvzColor* colors, float* diameters)
{
    ANN(positions);
    ANN(colors);
    ANN(diameters);

    ExampleRandom rng = example_random(SEED);
    for (uint32_t i = 0; i < point_count; i++)
    {
        positions[i][0] = example_random_range_f32(&rng, -1.0f, +1.0f);
        positions[i][1] = example_random_range_f32(&rng, -1.0f, +1.0f);
        positions[i][2] = 0.0f;

        colors[i].r = example_random_u8(&rng);
        colors[i].g = example_random_u8(&rng);
        colors[i].b = example_random_u8(&rng);
        colors[i].a = 200;

        diameters[i] = example_random_range_f32(&rng, 4.0f, 12.0f);
    }
}



/**
 * Add the retained scatter point visual.
 *
 * @param scene scene owning the visual
 * @param panel panel receiving the visual
 * @param point_count number of points to create
 * @return true when the visual was added
 */
static bool _add_points(DvzScene* scene, DvzPanel* panel, uint32_t point_count)
{
    ANN(scene);
    ANN(panel);

    bool ok = false;
    vec3* positions = (vec3*)dvz_calloc(point_count, sizeof(*positions));
    DvzColor* colors = (DvzColor*)dvz_calloc(point_count, sizeof(*colors));
    float* diameters = (float*)dvz_calloc(point_count, sizeof(*diameters));
    if (positions == NULL || colors == NULL || diameters == NULL)
        goto cleanup;

    _fill_scatter(point_count, positions, colors, diameters);

    DvzVisual* point = dvz_point(scene, 0);
    if (point == NULL)
        goto cleanup;

    DvzVisualDataUpdate updates[] = {
        {.attr_name = "position", .data = positions, .item_count = point_count},
        {.attr_name = "color", .data = colors, .item_count = point_count},
        {.attr_name = "diameter_px", .data = diameters, .item_count = point_count},
    };
    if (dvz_visual_set_data_many(point, updates, DVZ_ARRAY_COUNT(updates)) != 0)
        goto cleanup;

    DvzPointStyleDesc style = dvz_point_style_desc();
    style.aspect = DVZ_SHAPE_ASPECT_FILLED;
    style.stroke_width_px = 0.0f;
    if (dvz_point_set_style(point, &style) != 0)
        goto cleanup;
    if (dvz_visual_set_depth_test(point, false) != 0)
        goto cleanup;
    if (dvz_visual_set_alpha_mode(point, DVZ_ALPHA_BLENDED) != 0)
        goto cleanup;

    ok = dvz_panel_add_visual(panel, point, NULL) == 0;

cleanup:
    dvz_free(diameters);
    dvz_free(colors);
    dvz_free(positions);
    return ok;
}



/*************************************************************************************************/
/*  Scenario callbacks                                                                           */
/*************************************************************************************************/

static bool _scenario_init(DvzScenarioContext* ctx, void** out_user)
{
    if (ctx == NULL || out_user == NULL)
        return false;
    *out_user = NULL;

    ctx->figure = dvz_figure(ctx->scene, ctx->width, ctx->height, 0);
    if (ctx->figure == NULL)
        return false;

    DvzPanel* panel = dvz_panel_full(ctx->figure);
    if (panel == NULL)
        return false;
    example_graphite_cyan_set_panel_background(panel);

    uint32_t point_count = DEFAULT_POINT_COUNT;
    const char* point_count_env = getenv("DVZ_SCATTER_POINT_COUNT");
    if (point_count_env != NULL && point_count_env[0] != '\0')
    {
        char* end = NULL;
        unsigned long parsed = strtoul(point_count_env, &end, 10);
        if (end == point_count_env || *end != '\0' || parsed == 0 || parsed > MAX_POINT_COUNT)
        {
            fprintf(
                stderr, "scatter: invalid DVZ_SCATTER_POINT_COUNT '%s' (expected 1..%u)\n",
                point_count_env, MAX_POINT_COUNT);
            return false;
        }
        point_count = (uint32_t)parsed;
    }
    fprintf(stdout, "scenario_benchmark_points: %u\n", point_count);

    if (!_add_points(ctx->scene, panel, point_count))
        return false;

    DvzPanzoom* panzoom = dvz_scenario_panzoom(ctx, panel, NULL, DVZ_DIM_MASK_XY);
    if (panzoom == NULL)
        return false;

    ScatterState* state = (ScatterState*)dvz_calloc(1, sizeof(ScatterState));
    if (state == NULL)
        return false;
    state->panzoom = panzoom;

    const char* workload = getenv("DVZ_SCATTER_BENCHMARK");
    if (workload != NULL && workload[0] != '\0' && strcmp(workload, "static") != 0)
    {
        if (strcmp(workload, PANZOOM_WORKLOAD) != 0)
        {
            fprintf(stderr, "scatter: unsupported benchmark workload '%s'\n", workload);
            dvz_free(state);
            return false;
        }
        state->panzoom_benchmark = _panzoom_benchmark_requested();
        fprintf(stdout, "scenario_benchmark_workload: %s\n", PANZOOM_WORKLOAD);
    }

    *out_user = state;

    return true;
}



static void _scenario_frame(DvzScenarioContext* ctx, void* user_data)
{
    ScatterState* state = (ScatterState*)user_data;
    if (ctx == NULL || state == NULL || !state->panzoom_benchmark)
        return;

    const double tau = 6.28318530717958647692;
    const double phase =
        tau * (double)(ctx->frame_index % PANZOOM_PERIOD_FRAMES) / (double)PANZOOM_PERIOD_FRAMES;
    vec2 pan = {(float)(0.30 * sin(phase)), (float)(0.20 * cos(phase))};
    vec2 zoom = {
        (float)(1.55 + 0.35 * sin(phase + 0.35)),
        (float)(1.45 + 0.30 * cos(phase + 0.20)),
    };
    (void)dvz_panzoom_pan(state->panzoom, pan);
    (void)dvz_panzoom_zoom(state->panzoom, zoom);
}



static void _scenario_destroy(DvzScenarioContext* ctx, void* user_data)
{
    (void)ctx;
    dvz_free(user_data);
}

DvzScenarioSpec dvz_start_scatter_scenario(void)
{
    return (DvzScenarioSpec){
        .id = "start_scatter",
        .title = "Scatter Plot",
        .width = WIDTH,
        .height = HEIGHT,
        .fps = 60.0,
        .requirements =
            DVZ_SCENARIO_REQ_POINT_VISUAL | DVZ_SCENARIO_REQ_CONTROLLER | DVZ_SCENARIO_REQ_PANZOOM,
        .init = _scenario_init,
        .frame = _panzoom_benchmark_requested() ? _scenario_frame : NULL,
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
