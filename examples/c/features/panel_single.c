/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* panel_single - one explicit panel rectangle with panel chrome and one visual.
 *
 * Scenario: feature.panel_single
 * Style: features, graphite_cyan, 1280x720 window target
 *
 * Build:  just example-c features/panel_single
 * Run:    ./build/examples/c/features/panel_single --live
 * Smoke:  ./build/examples/c/features/panel_single --png
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
/*  Forward declarations                                                                         */
/*************************************************************************************************/

DvzScenarioSpec dvz_example_panel_single_scenario(void);



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH  EXAMPLE_WINDOW_WIDTH
#define HEIGHT EXAMPLE_WINDOW_HEIGHT
#define POINT_COUNT 5u



/*************************************************************************************************/
/*  Scenario callbacks                                                                           */
/*************************************************************************************************/

/**
 * Initialize the single-panel ownership and viewport scenario.
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
        {-0.56f, -0.18f, 0.0f}, {-0.28f, +0.20f, 0.0f}, {+0.00f, -0.06f, 0.0f},
        {+0.28f, +0.20f, 0.0f}, {+0.56f, -0.18f, 0.0f},
    };
    DvzColor colors[POINT_COUNT] = {
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ACCENT_PRIMARY),
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ACCENT_SECONDARY),
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_WARNING),
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ACCENT_SECONDARY),
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ACCENT_PRIMARY),
    };
    const float diameters[POINT_COUNT] = {28.0f, 38.0f, 52.0f, 38.0f, 28.0f};

    ctx->figure = dvz_figure(ctx->scene, ctx->width, ctx->height, 0);
    if (ctx->figure == NULL)
        return false;

    DvzPanel* panel =
        dvz_panel(ctx->figure, &(DvzPanelDesc){
                                   .x = 0.14f,
                                   .y = 0.16f,
                                   .width = 0.72f,
                                   .height = 0.68f,
                               });
    if (panel == NULL)
        return false;
    example_graphite_cyan_set_panel_background(panel);

    DvzPanelBorderDesc border = dvz_panel_border_desc();
    border.color = example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_GRID);
    border.width_px = 2.0f;
    if (dvz_panel_set_border(panel, &border) != DVZ_OK)
        return false;

    DvzVisual* point = dvz_point(ctx->scene, 0);
    if (point == NULL)
        return false;

    DvzVisualDataUpdate updates[] = {
        {.attr_name = "position", .data = positions, .item_count = POINT_COUNT},
        {.attr_name = "color", .data = colors, .item_count = POINT_COUNT},
        {.attr_name = "diameter_px", .data = diameters, .item_count = POINT_COUNT},
    };
    if (dvz_visual_set_data_many(point, updates, 3) != 0)
        return false;

    DvzPointStyleDesc style = dvz_point_style_desc();
    style.aspect = DVZ_SHAPE_ASPECT_FILLED;
    style.stroke_width_px = 0.0f;
    if (dvz_point_set_style(point, &style) != 0)
        return false;

    if (dvz_visual_set_depth_test(point, false) != 0)
        return false;

    return dvz_panel_add_visual(panel, point, NULL) == 0;
}



/**
 * Return the single-panel scenario specification.
 *
 * @return scenario specification
 */
DvzScenarioSpec dvz_example_panel_single_scenario(void)
{
    return (DvzScenarioSpec){
        .id = "feature_panel_single",
        .title = "Single Panel",
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
 * Run the single-panel ownership and viewport feature example through the native scenario runner.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @return process exit code
 */
#ifndef DVZ_EXAMPLE_NO_MAIN
int main(int argc, char** argv)
{
    DvzScenarioSpec spec = dvz_example_panel_single_scenario();
    return dvz_scenario_run_native_cli(&spec, argc, argv) == 0 ? 0 : 1;
}
#endif
