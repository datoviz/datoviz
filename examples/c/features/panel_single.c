/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* panel_single - one explicit panel rectangle with panel chrome and one visual.
 *
 * Scenario: feature.panel_single
 * Style: features, graphite_cyan, 1600x1200 capture target
 *
 * Build:  just example-c features/panel_single
 * Run:    ./build/examples/c/features/panel_single
 * Smoke:  ./build/examples/c/features/panel_single 1
 */



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

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
#define POINT_COUNT 5u



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Run the single-panel ownership and viewport feature example.
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

    scene = dvz_scene();
    EXAMPLE_CHECK(scene != NULL, "dvz_scene() failed");

    DvzFigure* figure = dvz_figure(scene, WIDTH, HEIGHT, 0);
    EXAMPLE_CHECK(figure != NULL, "dvz_figure() failed");

    DvzPanel* panel =
        dvz_panel(figure, (DvzPanelDesc){.x = 0.14f, .y = 0.16f, .width = 0.72f, .height = 0.68f});
    EXAMPLE_CHECK(panel != NULL, "dvz_panel() failed");
    example_graphite_cyan_set_panel_background(panel);

    DvzPanelBorderDesc border = dvz_panel_border_desc();
    border.color = example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_GRID);
    border.width_px = 2.0f;
    EXAMPLE_CHECK(dvz_panel_set_border(panel, &border), "dvz_panel_set_border() failed");

    DvzVisual* point = dvz_point(scene, 0);
    EXAMPLE_CHECK(point != NULL, "dvz_point() failed");

    DvzVisualDataUpdate updates[] = {
        {.attr_name = "position", .data = positions, .item_count = POINT_COUNT},
        {.attr_name = "color", .data = colors, .item_count = POINT_COUNT},
        {.attr_name = "diameter", .data = diameters, .item_count = POINT_COUNT},
    };
    int rc = dvz_visual_set_data_many(point, updates, 3);
    EXAMPLE_CHECK(rc == 0, "point data upload failed");

    DvzPointStyleDesc style = dvz_point_style_desc();
    style.aspect = DVZ_SHAPE_ASPECT_FILLED;
    style.stroke_width = 0.0f;
    rc = dvz_point_set_style(point, &style);
    EXAMPLE_CHECK(rc == 0, "dvz_point_set_style() failed");

    rc = dvz_visual_set_depth_test(point, false);
    EXAMPLE_CHECK(rc == 0, "dvz_visual_set_depth_test() failed");

    rc = dvz_panel_add_visual(panel, point, NULL);
    EXAMPLE_CHECK(rc == 0, "dvz_panel_add_visual() failed");

    app = dvz_app(scene);
    EXAMPLE_CHECK(app != NULL, "dvz_app() failed (no GPU or display?)");

    DvzView* win = dvz_view_glfw(app, figure, WIDTH, HEIGHT, "panel_single");
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
