/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* update_visual_data - retained point visual with full data replacement.
 *
 * Scenario: feature.update_visual_data
 * Style: features, graphite_cyan, 1600x1200 capture target
 *
 * Build:  just example-c features/update_visual_data
 * Run:    ./build/examples/c/features/update_visual_data
 * Smoke:  ./build/examples/c/features/update_visual_data 1
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
#define POINT_COUNT 7u



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Upload all point arrays to one retained point visual.
 *
 * @param visual point visual
 * @param positions point positions
 * @param colors point colors
 * @param diameters point diameters
 * @return true when the upload succeeds
 */
static bool _upload_points(
    DvzVisual* visual, const vec3 positions[POINT_COUNT], const DvzColor colors[POINT_COUNT],
    const float diameters[POINT_COUNT])
{
    DvzVisualDataUpdate updates[] = {
        {.attr_name = "position", .data = positions, .item_count = POINT_COUNT},
        {.attr_name = "color", .data = colors, .item_count = POINT_COUNT},
        {.attr_name = "diameter", .data = diameters, .item_count = POINT_COUNT},
    };
    return dvz_visual_set_data_many(visual, updates, 3) == 0;
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Run the full retained visual data update feature example.
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

    const vec3 initial_positions[POINT_COUNT] = {
        {-0.72f, -0.32f, 0.0f}, {-0.48f, -0.32f, 0.0f}, {-0.24f, -0.32f, 0.0f},
        {+0.00f, -0.32f, 0.0f}, {+0.24f, -0.32f, 0.0f}, {+0.48f, -0.32f, 0.0f},
        {+0.72f, -0.32f, 0.0f},
    };
    const vec3 updated_positions[POINT_COUNT] = {
        {-0.72f, +0.18f, 0.0f}, {-0.48f, -0.02f, 0.0f}, {-0.24f, +0.30f, 0.0f},
        {+0.00f, +0.04f, 0.0f}, {+0.24f, +0.30f, 0.0f}, {+0.48f, -0.02f, 0.0f},
        {+0.72f, +0.18f, 0.0f},
    };
    DvzColor initial_colors[POINT_COUNT] = {
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_GRID),
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_GRID),
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_GRID),
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_GRID),
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_GRID),
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_GRID),
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_GRID),
    };
    DvzColor updated_colors[POINT_COUNT] = {
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ACCENT_PRIMARY),
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ACCENT_SECONDARY),
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_WARNING),
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_TEXT),
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_WARNING),
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ACCENT_SECONDARY),
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ACCENT_PRIMARY),
    };
    const float initial_diameters[POINT_COUNT] = {18.0f, 18.0f, 18.0f, 18.0f, 18.0f, 18.0f, 18.0f};
    const float updated_diameters[POINT_COUNT] = {26.0f, 34.0f, 44.0f, 58.0f, 44.0f, 34.0f, 26.0f};

    scene = dvz_scene();
    EXAMPLE_CHECK(scene != NULL, "dvz_scene() failed");

    DvzFigure* figure = dvz_figure(scene, WIDTH, HEIGHT, 0);
    EXAMPLE_CHECK(figure != NULL, "dvz_figure() failed");

    DvzPanel* panel = dvz_panel_full(figure);
    EXAMPLE_CHECK(panel != NULL, "dvz_panel_full() failed");
    example_graphite_cyan_set_panel_background(panel);

    DvzVisual* point = dvz_point(scene, 0);
    EXAMPLE_CHECK(point != NULL, "dvz_point() failed");

    EXAMPLE_CHECK(
        _upload_points(point, initial_positions, initial_colors, initial_diameters),
        "initial point upload failed");
    EXAMPLE_CHECK(
        _upload_points(point, updated_positions, updated_colors, updated_diameters),
        "full point data replacement failed");

    DvzPointStyleDesc style = dvz_point_style_desc();
    style.aspect = DVZ_SHAPE_ASPECT_FILLED;
    style.stroke_width = 0.0f;
    int rc = dvz_point_set_style(point, &style);
    EXAMPLE_CHECK(rc == 0, "dvz_point_set_style() failed");

    rc = dvz_visual_set_depth_test(point, false);
    EXAMPLE_CHECK(rc == 0, "dvz_visual_set_depth_test() failed");

    rc = dvz_panel_add_visual(panel, point, NULL);
    EXAMPLE_CHECK(rc == 0, "dvz_panel_add_visual() failed");

    app = dvz_app(scene);
    EXAMPLE_CHECK(app != NULL, "dvz_app() failed (no GPU or display?)");

    DvzView* win = dvz_view_glfw(app, figure, WIDTH, HEIGHT, "update_visual_data");
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
