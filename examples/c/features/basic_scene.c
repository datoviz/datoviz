/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* basic_scene - smallest runner-backed retained scene with one point visual.
 *
 * Scenario: feature.basic_scene
 * Style: features, graphite_cyan, 1600x1200 capture target
 *
 * Build:  just example-c features/basic_scene
 * Run:    ./build/examples/c/features/basic_scene --live
 * Smoke:  ./build/examples/c/features/basic_scene --png
 */



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>

#include "datoviz/scene.h"
#include "example_style.h"
#include "runner/scenario_runner.h"



DvzScenarioSpec dvz_example_basic_scene_scenario(void);



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH       1600u
#define HEIGHT      1200u
#define POINT_COUNT 3u



/*************************************************************************************************/
/*  Scenario callbacks                                                                           */
/*************************************************************************************************/

/**
 * Initialize the smallest retained scene scenario.
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

    const vec3 positions[POINT_COUNT] = {
        {-0.45f, -0.25f, 0.0f},
        {+0.00f, +0.34f, 0.0f},
        {+0.45f, -0.25f, 0.0f},
    };
    DvzColor colors[POINT_COUNT] = {
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ACCENT_PRIMARY),
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ACCENT_SECONDARY),
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_WARNING),
    };
    const float diameters[POINT_COUNT] = {42.0f, 58.0f, 42.0f};

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

    DvzVisualDataUpdate updates[] = {
        {.attr_name = "position", .data = positions, .item_count = POINT_COUNT},
        {.attr_name = "color", .data = colors, .item_count = POINT_COUNT},
        {.attr_name = "diameter", .data = diameters, .item_count = POINT_COUNT},
    };
    if (dvz_visual_set_data_many(point, updates, 3) != 0)
        return false;

    DvzPointStyleDesc style = dvz_point_style_desc();
    style.aspect = DVZ_SHAPE_ASPECT_FILLED;
    style.stroke_width = 0.0f;
    if (dvz_point_set_style(point, &style) != 0)
        return false;
    if (dvz_visual_set_depth_test(point, false) != 0)
        return false;
    return dvz_panel_add_visual(panel, point, NULL) == 0;
}



/**
 * Return the scene-basic scenario specification.
 *
 * @return scenario specification
 */
DvzScenarioSpec dvz_example_basic_scene_scenario(void)
{
    return (DvzScenarioSpec){
        .id = "feature_basic_scene",
        .title = "basic_scene",
        .width = WIDTH,
        .height = HEIGHT,
        .fps = 60.0,
        .requirements = DVZ_SCENARIO_REQ_POINT_VISUAL,
        .init = _scenario_init,
    };
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Run the smallest retained scene feature example through the native scenario runner.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @return process exit code
 */
#ifndef DVZ_EXAMPLE_NO_MAIN
int main(int argc, char** argv)
{
    DvzScenarioSpec spec = dvz_example_basic_scene_scenario();
    return dvz_scenario_run_native_cli(&spec, argc, argv) == 0 ? 0 : 1;
}
#endif
