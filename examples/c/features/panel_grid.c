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
 * Run:    ./build/examples/c/features/panel_grid
 * Smoke:  ./build/examples/c/features/panel_grid 1
 */



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>

#include "datoviz/app.h"
#include "datoviz/scene.h"
#include "example_common.h"
#include "example_style.h"



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



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Run the grid layout feature example.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @return process exit code
 */
int main(int argc, char** argv)
{
    int ret = 1;
    DvzScene* scene = NULL;
    DvzApp* app = NULL;

    scene = dvz_scene();
    EXAMPLE_CHECK(scene != NULL, "dvz_scene() failed");

    DvzFigure* figure = dvz_figure(scene, WIDTH, HEIGHT, 0);
    EXAMPLE_CHECK(figure != NULL, "dvz_figure() failed");

    DvzGrid* grid = dvz_figure_grid(figure, 2, 2);
    EXAMPLE_CHECK(grid != NULL, "dvz_figure_grid() failed");
    EXAMPLE_CHECK(
        dvz_grid_set_margins(
            grid, &(DvzPanelReserve){
                      .left_px = 90.0f, .right_px = 90.0f, .top_px = 80.0f, .bottom_px = 80.0f}),
        "dvz_grid_set_margins() failed");
    EXAMPLE_CHECK(dvz_grid_set_gutter(grid, 36.0f, 36.0f), "dvz_grid_set_gutter() failed");

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
        EXAMPLE_CHECK(panels[i] != NULL, "dvz_grid_panel() failed");
        example_graphite_cyan_set_panel_background(panels[i]);

        DvzPanelBorderDesc border = dvz_panel_border_desc();
        border.color = example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_GRID);
        border.width_px = 1.5f;
        EXAMPLE_CHECK(dvz_panel_set_border(panels[i], &border), "dvz_panel_set_border() failed");

        EXAMPLE_CHECK(_add_panel_points(scene, panels[i], accents[i]), "panel point setup failed");
    }

    app = dvz_app(scene);
    EXAMPLE_CHECK(app != NULL, "dvz_app() failed (no GPU or display?)");

    DvzView* win = dvz_view_glfw(app, figure, WIDTH, HEIGHT, "panel_grid");
    EXAMPLE_CHECK(win != NULL, "dvz_view_glfw() failed (GLFW unavailable?)");

    dvz_app_run(app, example_frame_count_any(argc, argv));
    ret = 0;

cleanup:
    if (app != NULL)
        dvz_app_destroy(app);
    if (scene != NULL)
        dvz_scene_destroy(scene);
    return ret;
}
