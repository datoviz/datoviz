/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* colormap_scale - point visual with scalar float colors and one retained color scale.
 *
 * Scenario: feature.colormap_scale
 * Style: features, graphite_cyan, 1600x1200 capture target
 *
 * Build:  just example-c features/colormap_scale
 * Run:    ./build/examples/c/features/colormap_scale --live
 * Smoke:  ./build/examples/c/features/colormap_scale --png
 */



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>

#include "datoviz/scene.h"
#include "example_style.h"
#include "runner/scenario_runner.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH       1600u
#define HEIGHT      1200u
#define POINT_COUNT 5u



/*************************************************************************************************/
/*  Scenario callbacks                                                                           */
/*************************************************************************************************/

/**
 * Initialize the scalar color scale point-visual scenario.
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

    vec3 positions[POINT_COUNT] = {
        {-0.62f, -0.18f, 0.0f}, {-0.30f, +0.18f, 0.0f}, {+0.00f, -0.08f, 0.0f},
        {+0.30f, +0.24f, 0.0f}, {+0.62f, -0.14f, 0.0f},
    };
    float values[POINT_COUNT] = {0.05f, 0.30f, 0.52f, 0.74f, 0.96f};
    float diameters[POINT_COUNT] = {48.0f, 56.0f, 64.0f, 56.0f, 48.0f};

    ctx->figure = dvz_figure(ctx->scene, ctx->width, ctx->height, 0);
    if (ctx->figure == NULL)
        return false;

    DvzPanel* panel = dvz_panel_full(ctx->figure);
    if (panel == NULL)
        return false;
    example_graphite_cyan_set_panel_background(panel);

    DvzVisual* point = dvz_point(ctx->scene, 0);
    if (point == NULL)
        return false;

    int rc = dvz_visual_set_attr_format(point, "color", DVZ_VISUAL_ATTR_FORMAT_SCALAR_F32);
    if (rc != 0)
        return false;

    DvzScale* scale = example_graphite_cyan_color_scale(ctx->scene, 0.0, 1.0);
    if (scale == NULL)
        return false;

    rc = dvz_visual_set_scale(point, "color", scale);
    if (rc != 0)
        return false;

    DvzVisualDataUpdate updates[] = {
        {.attr_name = "position", .data = positions, .item_count = POINT_COUNT},
        {.attr_name = "color", .data = values, .item_count = POINT_COUNT},
        {.attr_name = "diameter", .data = diameters, .item_count = POINT_COUNT},
    };
    rc = dvz_visual_set_data_many(point, updates, 3);
    if (rc != 0)
        return false;

    DvzPointStyleDesc style = dvz_point_style_desc();
    style.aspect = DVZ_SHAPE_ASPECT_FILLED;
    style.stroke_width = 0.0f;
    rc = dvz_point_set_style(point, &style);
    if (rc != 0)
        return false;

    rc = dvz_visual_set_depth_test(point, false);
    if (rc != 0)
        return false;

    rc = dvz_panel_add_visual(panel, point, NULL);
    return rc == 0;
}



/**
 * Return the colormap-scale scenario specification.
 *
 * @return scenario specification
 */
static DvzScenarioSpec _colormap_scale_scenario(void)
{
    return (DvzScenarioSpec){
        .id = "feature_colormap_scale",
        .title = "colormap_scale",
        .width = WIDTH,
        .height = HEIGHT,
        .fps = 60.0,
        .init = _scenario_init,
    };
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Run a minimal scalar color scale point-visual example through the native scenario runner.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @return process exit code
 */
int main(int argc, char** argv)
{
    DvzScenarioSpec spec = _colormap_scale_scenario();
    return dvz_scenario_run_native_cli(&spec, argc, argv) == 0 ? 0 : 1;
}
