/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* depth_test - side-by-side visual depth-test toggle with overlapping 3D points.
 *
 * Scenario: feature.depth_test
 * Style: features, graphite_cyan, 1600x1200 capture target
 *
 * One panel keeps depth testing enabled. The other disables depth testing on the same retained
 * point visual, so the later far point overdraws the nearer point.
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
#define POINT_COUNT 2u



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Add two overlapping depth-separated points to one panel.
 *
 * @param scene scene owning the visual
 * @param panel panel receiving the visual
 * @param depth_test_enabled whether the visual should test scene depth
 * @return true when the visual was added
 */
static bool _add_depth_points(DvzScene* scene, DvzPanel* panel, bool depth_test_enabled)
{
    const vec3 positions[POINT_COUNT] = {
        {-0.06f, -0.34f, 0.02f},
        {+0.07f, +0.36f, 0.02f},
    };
    float diameters[POINT_COUNT] = {260.0f, 260.0f};
    DvzColor colors[POINT_COUNT] = {
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ACCENT_PRIMARY),
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_WARNING),
    };
    colors[0].a = 248;
    colors[1].a = 248;

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

    if (dvz_visual_set_depth_test(point, depth_test_enabled) != 0)
        return false;

    return dvz_panel_add_visual(panel, point, NULL) == 0;
}



/**
 * Apply the shared camera used by both comparison panels.
 *
 * @param panel target panel
 * @return true when the camera was applied
 */
static bool _set_depth_camera(DvzPanel* panel)
{
    DvzCameraDesc camera_desc = dvz_camera_desc();
    camera_desc.eye[0] = 0.00f;
    camera_desc.eye[1] = -3.10f;
    camera_desc.eye[2] = 0.38f;
    camera_desc.up[1] = 0.0f;
    camera_desc.up[2] = 1.0f;
    camera_desc.fov_y = 0.50f;
    camera_desc.near = 0.05f;
    camera_desc.far = 100.0f;
    return dvz_panel_set_camera(panel, &camera_desc) != NULL;
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Run the visual depth-test feature example.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @return process exit code
 */
int main(int argc, char** argv)
{
    const uint32_t frame_count = example_frame_count_any(argc, argv);
    DvzAppCaptureConfig capture = dvz_app_capture_config_from_env("feature_depth_test");

    int ret = 1;
    DvzScene* scene = NULL;
    DvzApp* app = NULL;
    DvzView* win = NULL;

    scene = dvz_scene();
    EXAMPLE_CHECK(scene != NULL, "dvz_scene() failed");

    DvzFigure* figure = dvz_figure(scene, WIDTH, HEIGHT, 0);
    EXAMPLE_CHECK(figure != NULL, "dvz_figure() failed");

    DvzGrid* grid = dvz_figure_grid(figure, 1, 2);
    EXAMPLE_CHECK(grid != NULL, "dvz_figure_grid() failed");
    EXAMPLE_CHECK(
        dvz_grid_set_margins(
            grid,
            &(DvzPanelReserve){
                .left_px = 42.0f, .right_px = 42.0f, .top_px = 38.0f, .bottom_px = 38.0f}),
        "dvz_grid_set_margins() failed");
    EXAMPLE_CHECK(dvz_grid_set_gutter(grid, 30.0f, 0.0f), "dvz_grid_set_gutter() failed");

    DvzPanel* depth_on = dvz_grid_panel(grid, 0, 0);
    DvzPanel* depth_off = dvz_grid_panel(grid, 0, 1);
    EXAMPLE_CHECK(depth_on != NULL && depth_off != NULL, "dvz_grid_panel() failed");
    example_graphite_cyan_set_panel_background(depth_on);
    example_graphite_cyan_set_panel_background(depth_off);

    EXAMPLE_CHECK(_set_depth_camera(depth_on), "left panel camera setup failed");
    EXAMPLE_CHECK(_set_depth_camera(depth_off), "right panel camera setup failed");
    EXAMPLE_CHECK(_add_depth_points(scene, depth_on, true), "depth-tested point setup failed");
    EXAMPLE_CHECK(
        _add_depth_points(scene, depth_off, false), "non-depth-tested point setup failed");

    app = dvz_app(scene);
    EXAMPLE_CHECK(app != NULL, "dvz_app() failed (no GPU or display?)");

    win = dvz_view_glfw(app, figure, WIDTH, HEIGHT, "feature_depth_test");
    EXAMPLE_CHECK(win != NULL, "dvz_view_glfw() failed (GLFW unavailable?)");

    EXAMPLE_CHECK(
        example_run_with_capture(app, win, frame_count, &capture),
        "example_run_with_capture() failed");
    ret = 0;

cleanup:
    if (app != NULL)
        dvz_app_destroy(app);
    if (scene != NULL)
        dvz_scene_destroy(scene);
    return ret;
}
