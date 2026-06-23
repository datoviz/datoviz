/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* panel_grid - four grid-owned panels with clipped panel-local content.
 *
 * Scenario: feature.panel_grid
 * Style: features, graphite_cyan, 1600x1200 capture target
 *
 * Build:  just example-c features/panel_grid
 * Run:    ./build/examples/c/features/panel_grid --live
 * Smoke:  ./build/examples/c/features/panel_grid --png
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

DvzScenarioSpec dvz_example_panel_grid_scenario(void);



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH       1600u
#define HEIGHT      1200u
#define PANEL_COUNT 4u
#define POINT_COUNT 4u



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Add one small point group to a grid panel.
 *
 * @param scene scene owning the visual
 * @param panel panel receiving the visual
 * @param accent main color for this panel
 * @return true when the visual was added
 */
static bool _add_panel_points(DvzScene* scene, DvzPanel* panel, DvzColor accent)
{
    const vec3 positions[POINT_COUNT] = {
        {-0.35f, -0.28f, 0.0f},
        {+0.35f, -0.28f, 0.0f},
        {+0.00f, +0.32f, 0.0f},
        {+0.00f, +0.00f, 0.0f},
    };
    DvzColor colors[POINT_COUNT] = {
        accent,
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ACCENT_SECONDARY),
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_TEXT),
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_WARNING),
    };
    const float diameters[POINT_COUNT] = {26.0f, 26.0f, 34.0f, 12.0f};

    DvzVisual* point = dvz_point(scene, 0);
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



/*************************************************************************************************/
/*  Scenario callbacks                                                                           */
/*************************************************************************************************/

/**
 * Initialize the grid layout scenario.
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

    DvzGrid* grid = dvz_figure_grid(ctx->figure, 2, 2);
    if (grid == NULL)
        return false;
    if (!dvz_grid_set_margins(
            grid,
            &(DvzPanelReserve){
                .left_px = 90.0f, .right_px = 90.0f, .top_px = 80.0f, .bottom_px = 80.0f}))
        return false;
    if (!dvz_grid_set_gutter(grid, 36.0f, 36.0f))
        return false;

    DvzPanel* panels[PANEL_COUNT] = {
        dvz_grid_panel(grid, 0, 0),
        dvz_grid_panel(grid, 0, 1),
        dvz_grid_panel(grid, 1, 0),
        dvz_grid_panel(grid, 1, 1),
    };
    DvzColor accents[PANEL_COUNT] = {
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ACCENT_PRIMARY),
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ACCENT_SECONDARY),
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_WARNING),
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ERROR),
    };

    for (uint32_t i = 0; i < PANEL_COUNT; i++)
    {
        if (panels[i] == NULL)
            return false;
        example_graphite_cyan_set_panel_background(panels[i]);

        DvzPanelBorderDesc border = dvz_panel_border_desc();
        border.color = example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_GRID);
        border.width_px = 1.5f;
        if (!dvz_panel_set_border(panels[i], &border))
            return false;

        if (!_add_panel_points(ctx->scene, panels[i], accents[i]))
            return false;
    }

    return true;
}



/**
 * Return the panel-grid scenario specification.
 *
 * @return scenario specification
 */
DvzScenarioSpec dvz_example_panel_grid_scenario(void)
{
    return (DvzScenarioSpec){
        .id = "feature_panel_grid",
        .title = "panel_grid",
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
 * Run the grid layout feature example through the native scenario runner.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @return process exit code
 */
#ifndef DVZ_EXAMPLE_NO_MAIN
int main(int argc, char** argv)
{
    DvzScenarioSpec spec = dvz_example_panel_grid_scenario();
    return dvz_scenario_run_native_cli(&spec, argc, argv) == 0 ? 0 : 1;
}
#endif
