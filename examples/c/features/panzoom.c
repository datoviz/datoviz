/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* panzoom - bind a panzoom controller to one panel with a simple 2D visual.
 *
 * Scenario: feature.panzoom
 * Style: features, graphite_cyan, 1280x720 window target
 *
 * Build:  just example-c features/panzoom
 * Run:    ./build/examples/c/features/panzoom --live
 * Smoke:  ./build/examples/c/features/panzoom --png
 */



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <math.h>
#include <stdbool.h>
#include <stdint.h>

#include "_assertions.h"
#include "datoviz/scene.h"
#include "example_style.h"
#include "runner/scenario_runner.h"



/*************************************************************************************************/
/*  Forward declarations                                                                         */
/*************************************************************************************************/

DvzScenarioSpec dvz_example_panzoom_scenario(void);



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH  EXAMPLE_WINDOW_WIDTH
#define HEIGHT EXAMPLE_WINDOW_HEIGHT
#define POINT_COUNT 64u

static const float TAU = 6.28318530718f;



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Fill a deterministic 2D point ring in panel data coordinates.
 *
 * @param positions output data-space positions
 * @param colors output point colors
 * @param diameters output point diameters
 */
static void _fill_points(
    vec3 positions[POINT_COUNT], DvzColor colors[POINT_COUNT], float diameters[POINT_COUNT])
{
    ANN(positions);
    ANN(colors);
    ANN(diameters);

    DvzColor primary = example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ACCENT_PRIMARY);
    DvzColor secondary = example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ACCENT_SECONDARY);
    for (uint32_t i = 0; i < POINT_COUNT; i++)
    {
        const float t = (float)i / (float)POINT_COUNT;
        const float theta = TAU * t;
        const float radius = 0.42f + 0.18f * sinf(5.0f * theta);

        positions[i][0] = radius * cosf(theta);
        positions[i][1] = radius * sinf(theta);
        positions[i][2] = 0.0f;

        colors[i] = i % 2u == 0u ? primary : secondary;
        colors[i].a = 230u;
        diameters[i] = 8.0f + 5.0f * (0.5f + 0.5f * sinf(3.0f * theta));
    }
}



/**
 * Add the point visual controlled by the panel panzoom.
 *
 * @param scene scene owning the visual
 * @param panel panel receiving the visual
 * @return true when the visual was added
 */
static bool _add_points(DvzScene* scene, DvzPanel* panel)
{
    ANN(scene);
    ANN(panel);

    vec3 data_positions[POINT_COUNT] = {{0}};
    DvzColor colors[POINT_COUNT] = {{0}};
    float diameters[POINT_COUNT] = {0};

    _fill_points(data_positions, colors, diameters);

    DvzVisual* visual = dvz_point(scene, 0);
    if (visual == NULL)
        return false;

    DvzVisualDataUpdate updates[] = {
        {.attr_name = "position", .data = data_positions, .item_count = POINT_COUNT},
        {.attr_name = "color", .data = colors, .item_count = POINT_COUNT},
        {.attr_name = "diameter_px", .data = diameters, .item_count = POINT_COUNT},
    };
    if (dvz_visual_set_data_many(visual, updates, 3) != 0)
        return false;

    DvzPointStyleDesc style = dvz_point_style_desc();
    style.aspect = DVZ_SHAPE_ASPECT_FILLED;
    style.stroke_width_px = 0.0f;
    if (dvz_point_set_style(visual, &style) != 0)
        return false;
    if (dvz_visual_set_depth_test(visual, false) != 0)
        return false;

    return dvz_panel_add_visual(panel, visual, NULL) == 0;
}



/*************************************************************************************************/
/*  Scenario callbacks                                                                           */
/*************************************************************************************************/

/**
 * Initialize the panzoom attachment feature scenario.
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

    ctx->figure = dvz_figure(ctx->scene, ctx->width, ctx->height, 0);
    if (ctx->figure == NULL)
        return false;

    DvzPanel* panel = dvz_panel_full(ctx->figure);
    if (panel == NULL)
        return false;
    example_graphite_cyan_set_panel_background(panel);

    int rc = dvz_panel_set_domain(panel, DVZ_DIM_X, -1.0, 1.0);
    if (rc != 0)
        return false;
    rc = dvz_panel_set_domain(panel, DVZ_DIM_Y, -1.0, 1.0);
    if (rc != 0)
        return false;

    if (!_add_points(ctx->scene, panel))
        return false;

    DvzController* controller = dvz_panzoom(ctx->scene, NULL);
    if (controller == NULL)
        return false;
    DvzPanzoom* panzoom = dvz_controller_panzoom(controller);
    return panzoom != NULL &&
           dvz_scenario_bind_controller(ctx, panel, controller, DVZ_DIM_MASK_XY) == 0;
}



/**
 * Return the panzoom attachment scenario specification.
 *
 * @return scenario specification
 */
DvzScenarioSpec dvz_example_panzoom_scenario(void)
{
    return (DvzScenarioSpec){
        .id = "feature_panzoom",
        .title = "panzoom",
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

/**
 * Run the panzoom attachment feature example through the native scenario runner.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @return process exit code
 */
#ifndef DVZ_EXAMPLE_NO_MAIN
int main(int argc, char** argv)
{
    DvzScenarioSpec spec = dvz_example_panzoom_scenario();
    return dvz_scenario_run_native_cli(&spec, argc, argv) == 0 ? 0 : 1;
}
#endif
