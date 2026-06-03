/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* visibility - retained visual visibility toggled before rendering.
 *
 * Scenario: feature.visibility
 * Style: features, graphite_cyan, 1600x1200 capture target
 *
 * Build:  just example-c features/visibility
 * Run:    ./build/examples/c/features/visibility
 * Smoke:  ./build/examples/c/features/visibility 1
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
#define POINT_COUNT 1u



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Add one single-point visual.
 *
 * @param scene scene owning the visual
 * @param panel panel receiving the visual
 * @param x point X position
 * @param color point color
 * @param visible whether the visual should render
 * @return true when the visual was added
 */
static bool _add_point_visual(
    DvzScene* scene, DvzPanel* panel, float x, DvzColor color, bool visible)
{
    const vec3 positions[POINT_COUNT] = {{x, 0.0f, 0.0f}};
    const DvzColor colors[POINT_COUNT] = {color};
    const float diameters[POINT_COUNT] = {72.0f};

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

    dvz_visual_set_visible(point, visible);
    return dvz_panel_add_visual(panel, point, NULL) == 0;
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Run the retained visual visibility feature example.
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

    DvzPanel* panel = dvz_panel_full(figure);
    EXAMPLE_CHECK(panel != NULL, "dvz_panel_full() failed");
    example_graphite_cyan_set_panel_background(panel);

    EXAMPLE_CHECK(
        _add_point_visual(
            scene, panel, -0.42f, example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ACCENT_PRIMARY),
            true),
        "left visible visual setup failed");
    EXAMPLE_CHECK(
        _add_point_visual(
            scene, panel, 0.0f, example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ERROR), false),
        "hidden visual setup failed");
    EXAMPLE_CHECK(
        _add_point_visual(
            scene, panel, +0.42f, example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ACCENT_SECONDARY),
            true),
        "right visible visual setup failed");

    app = dvz_app(scene);
    EXAMPLE_CHECK(app != NULL, "dvz_app() failed (no GPU or display?)");

    DvzView* win = dvz_view_glfw(app, figure, WIDTH, HEIGHT, "visibility");
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
