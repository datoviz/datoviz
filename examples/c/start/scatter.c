/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* scatter - quickstart scatter plot: 10 000 random colored points with pan/zoom.
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

#include <stdbool.h>
#include <stdint.h>

#include "_alloc.h"
#include "_assertions.h"
#include "datoviz/scene.h"
#include "example_random.h"
#include "example_style.h"
#include "runner/scenario_runner.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH  EXAMPLE_WINDOW_WIDTH
#define HEIGHT EXAMPLE_WINDOW_HEIGHT
#define POINT_COUNT 10000u
#define SEED        12345u



/*************************************************************************************************/
/*  Forward declarations                                                                         */
/*************************************************************************************************/

DvzScenarioSpec dvz_start_scatter_scenario(void);



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

static void _fill_scatter(
    vec3* positions, DvzColor* colors, float* diameters)
{
    ANN(positions);
    ANN(colors);
    ANN(diameters);

    ExampleRandom rng = example_random(SEED);
    for (uint32_t i = 0; i < POINT_COUNT; i++)
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
 * @return true when the visual was added
 */
static bool _add_points(DvzScene* scene, DvzPanel* panel)
{
    ANN(scene);
    ANN(panel);

    bool ok = false;
    vec3* positions = (vec3*)dvz_calloc(POINT_COUNT, sizeof(*positions));
    DvzColor* colors = (DvzColor*)dvz_calloc(POINT_COUNT, sizeof(*colors));
    float* diameters = (float*)dvz_calloc(POINT_COUNT, sizeof(*diameters));
    if (positions == NULL || colors == NULL || diameters == NULL)
        goto cleanup;

    _fill_scatter(positions, colors, diameters);

    DvzVisual* point = dvz_point(scene, 0);
    if (point == NULL)
        goto cleanup;

    DvzVisualDataUpdate updates[] = {
        {.attr_name = "position", .data = positions, .item_count = POINT_COUNT},
        {.attr_name = "color", .data = colors, .item_count = POINT_COUNT},
        {.attr_name = "diameter_px", .data = diameters, .item_count = POINT_COUNT},
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
    if (ctx == NULL)
        return false;
    if (out_user != NULL)
        *out_user = NULL;

    ctx->figure = dvz_figure(ctx->scene, ctx->width, ctx->height, 0);
    if (ctx->figure == NULL)
        return false;

    DvzPanel* panel = dvz_panel_full(ctx->figure);
    if (panel == NULL)
        return false;
    example_graphite_cyan_set_panel_background(panel);

    if (!_add_points(ctx->scene, panel))
        return false;

    if (dvz_scenario_panzoom(ctx, panel, NULL, DVZ_DIM_MASK_XY) == NULL)
        return false;

    return true;
}

DvzScenarioSpec dvz_start_scatter_scenario(void)
{
    return (DvzScenarioSpec){
        .id = "start_scatter",
        .title = "Scatter Plot",
        .width = WIDTH,
        .height = HEIGHT,
        .fps = 60.0,
        .requirements = DVZ_SCENARIO_REQ_POINT_VISUAL | DVZ_SCENARIO_REQ_CONTROLLER |
                        DVZ_SCENARIO_REQ_PANZOOM,
        .init = _scenario_init,
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
