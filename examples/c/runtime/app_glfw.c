/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* app_glfw - This example opens a native GLFW window for a small retained point scene.
 *
 * What to look for: the scene creates one figure, one full-panel view, and one point visual with
 * explicit position, color, and diameter arrays. The live preview should show six differently
 * sized points on the graphite/cyan background, and panzoom interaction should work because the
 * view binds a controller after the GLFW window is created.
 *
 * Use this as the beginner native-app template when you want Datoviz to own the GLFW lifecycle
 * directly instead of running through the portable scenario runner.
 *
 * Scenario: feature.app_glfw
 * Style: runtime, native app
 *
 * Build:  just example-c runtime/app_glfw
 * Run:    ./build/examples/c/runtime/app_glfw
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

#define WIDTH  EXAMPLE_WINDOW_WIDTH
#define HEIGHT EXAMPLE_WINDOW_HEIGHT
#define POINT_COUNT 6u



/*************************************************************************************************/
/*  Main                                                                                         */
/*************************************************************************************************/

int main(int argc, char** argv)
{
    int ret = 1;
    DvzScene* scene = NULL;
    DvzApp* app = NULL;

    scene = dvz_scene();
    EXAMPLE_CHECK(scene != NULL, "dvz_scene() failed");

    DvzFigure* figure = dvz_figure(scene, WIDTH, HEIGHT, 0);
    DvzPanel* panel = figure != NULL ? dvz_panel_full(figure) : NULL;
    DvzVisual* point = dvz_point(scene, 0);
    EXAMPLE_CHECK(
        figure != NULL && panel != NULL && point != NULL, "failed to create scene objects");
    example_graphite_cyan_set_panel_background(panel);

    vec3 positions[POINT_COUNT] = {
        {-0.70f, -0.35f, 0.0f}, {-0.42f, +0.28f, 0.0f}, {-0.14f, -0.12f, 0.0f},
        {+0.14f, +0.42f, 0.0f}, {+0.42f, -0.22f, 0.0f}, {+0.70f, +0.18f, 0.0f},
    };
    DvzColor colors[POINT_COUNT] = {
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ACCENT_PRIMARY),
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ACCENT_SECONDARY),
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_WARNING),
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_TEXT),
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_GRID),
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_MINOR_TICK),
    };
    float diameters[POINT_COUNT] = {24.0f, 32.0f, 42.0f, 54.0f, 42.0f, 32.0f};
    DvzVisualDataUpdate updates[] = {
        {.attr_name = "position", .data = positions, .item_count = POINT_COUNT},
        {.attr_name = "color", .data = colors, .item_count = POINT_COUNT},
        {.attr_name = "diameter_px", .data = diameters, .item_count = POINT_COUNT},
    };
    EXAMPLE_CHECK(dvz_visual_set_data_many(point, updates, 3) == 0, "failed to upload point data");
    EXAMPLE_CHECK(dvz_panel_add_visual(panel, point, NULL) == 0, "dvz_panel_add_visual() failed");

    app = dvz_app(scene);
    EXAMPLE_CHECK(app != NULL, "dvz_app() failed (no GPU or display?)");

    DvzView* view = dvz_view_window(app, figure, WIDTH, HEIGHT, "app_glfw");
    EXAMPLE_CHECK(view != NULL, "dvz_view_window() failed (GLFW unavailable?)");
    EXAMPLE_CHECK(dvz_view_panzoom(view, panel, NULL) != NULL, "dvz_view_panzoom() failed");

    dvz_app_run(app, example_frame_count(argc, argv));
    ret = 0;

cleanup:
    if (app != NULL)
        dvz_app_destroy(app);
    if (scene != NULL)
        dvz_scene_destroy(scene);
    return ret;
}
