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
 * Run:    ./build/examples/c/features/colormap_scale
 * Smoke:  ./build/examples/c/features/colormap_scale 1
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
 * Run a minimal scalar color scale point-visual example.
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

    vec3 positions[POINT_COUNT] = {
        {-0.62f, -0.18f, 0.0f}, {-0.30f, +0.18f, 0.0f}, {+0.00f, -0.08f, 0.0f},
        {+0.30f, +0.24f, 0.0f}, {+0.62f, -0.14f, 0.0f},
    };
    float values[POINT_COUNT] = {0.05f, 0.30f, 0.52f, 0.74f, 0.96f};
    float diameters[POINT_COUNT] = {48.0f, 56.0f, 64.0f, 56.0f, 48.0f};

    scene = dvz_scene();
    EXAMPLE_CHECK(scene != NULL, "dvz_scene() failed");

    DvzFigure* figure = dvz_figure(scene, WIDTH, HEIGHT, 0);
    EXAMPLE_CHECK(figure != NULL, "dvz_figure() failed");

    DvzPanel* panel = dvz_panel_full(figure);
    EXAMPLE_CHECK(panel != NULL, "dvz_panel_full() failed");
    example_graphite_cyan_set_panel_background(panel);

    DvzVisual* point = dvz_point(scene, 0);
    EXAMPLE_CHECK(point != NULL, "dvz_point() failed");

    int rc = dvz_visual_set_attr_format(point, "color", DVZ_VISUAL_ATTR_FORMAT_SCALAR_F32);
    EXAMPLE_CHECK(rc == 0, "dvz_visual_set_attr_format() failed");

    DvzScale* scale = example_graphite_cyan_color_scale(scene, 0.0, 1.0);
    EXAMPLE_CHECK(scale != NULL, "example_graphite_cyan_color_scale() failed");

    rc = dvz_visual_set_scale(point, "color", scale);
    EXAMPLE_CHECK(rc == 0, "dvz_visual_set_scale() failed");

    DvzVisualDataUpdate updates[] = {
        {.attr_name = "position", .data = positions, .item_count = POINT_COUNT},
        {.attr_name = "color", .data = values, .item_count = POINT_COUNT},
        {.attr_name = "diameter", .data = diameters, .item_count = POINT_COUNT},
    };
    rc = dvz_visual_set_data_many(point, updates, 3);
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

    DvzView* win = dvz_view_glfw(app, figure, WIDTH, HEIGHT, "colormap_scale");
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
