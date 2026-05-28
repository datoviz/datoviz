/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* axes_2d - deterministic path with retained 2D axes and tick labels.
 *
 * Scenario: path_axes_2d
 * Style: features, graphite_cyan, 1280x960 capture target
 *
 * Build:  just example-c features/axes_2d
 * Run:    ./build/examples/c/features/axes_2d
 * Smoke:  ./build/examples/c/features/axes_2d 1
 */



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "_assertions.h"
#include "_compat.h"
#include "datoviz/app.h"
#include "datoviz/scene.h"
#include "example_common.h"
#include "example_style.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH      1280u
#define HEIGHT     960u
#define PATH_COUNT 384u

static const float TAU = 6.28318530718f;



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Fill deterministic path samples in data coordinates.
 *
 * @param positions output data-space path positions
 * @param colors output path colors
 * @param widths output path stroke widths in pixels
 * @param count sample count
 */
static void _fill_curve(vec3* positions, DvzColor* colors, float* widths, uint32_t count)
{
    ANN(positions);
    ANN(colors);
    ANN(widths);

    const float inv_count = count > 1 ? 1.0f / (float)(count - 1u) : 1.0f;
    for (uint32_t i = 0; i < count; i++)
    {
        const float t = (float)i * inv_count;
        const float x = 10.0f * t;
        const float envelope = expf(-0.12f * x);
        const float y = envelope * (1.65f * sinf(1.25f * TAU * t) +
                                    0.35f * sinf(4.0f * TAU * t + 0.35f));

        positions[i][0] = x;
        positions[i][1] = y;
        positions[i][2] = 0.0f;

        const uint8_t r = (uint8_t)(70.0f + 55.0f * t);
        const uint8_t g = (uint8_t)(196.0f + 38.0f * t);
        const uint8_t b = (uint8_t)(214.0f + 34.0f * (1.0f - t));
        colors[i] = dvz_color_rgba(r, g, b, 255);
        widths[i] = 4.0f;
    }
}



/**
 * Upload a single stroked path to a retained path visual.
 *
 * @param visual path visual
 * @param positions visual-space path positions
 * @param colors path colors
 * @param widths path stroke widths
 * @param count sample count
 * @return true when all uploads succeed
 */
static bool _upload_path(
    DvzVisual* visual, vec3* positions, DvzColor* colors, float* widths, uint32_t count)
{
    ANN(visual);
    ANN(positions);
    ANN(colors);
    ANN(widths);

    DvzVisualDataUpdate updates[] = {
        {.attr_name = "position", .data = positions, .item_count = count},
        {.attr_name = "color", .data = colors, .item_count = count},
        {.attr_name = "stroke_width", .data = widths, .item_count = count},
    };
    if (dvz_visual_set_data_many(visual, updates, 3) != 0)
        return false;
    if (dvz_path_set_caps(visual, DVZ_SEGMENT_CAP_ROUND, DVZ_SEGMENT_CAP_ROUND) != 0)
        return false;
    return dvz_path_set_join(visual, DVZ_PATH_JOIN_ROUND, 4.0f) == 0;
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Run the deterministic 2D axes and path feature proof.
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
    vec3 data_positions[PATH_COUNT] = {{0}};
    vec3 visual_positions[PATH_COUNT] = {{0}};
    DvzColor colors[PATH_COUNT] = {{0}};
    float widths[PATH_COUNT] = {0};

    _fill_curve(data_positions, colors, widths, PATH_COUNT);

    scene = dvz_scene();
    EXAMPLE_CHECK(scene != NULL, "dvz_scene() failed");

    DvzFigure* figure = dvz_figure(scene, WIDTH, HEIGHT, 0);
    EXAMPLE_CHECK(figure != NULL, "dvz_figure() failed");

    DvzPanel* panel = dvz_panel_full(figure);
    EXAMPLE_CHECK(panel != NULL, "dvz_panel_full() failed");
    example_graphite_cyan_set_panel_background(panel);

    bool ok = dvz_panel_set_layout_reserve(
        panel, &(DvzPanelLayoutReserve){.left = 0.16f, .right = 0.05f, .bottom = 0.15f,
                                        .top = 0.05f});
    EXAMPLE_CHECK(ok, "dvz_panel_set_layout_reserve() failed");

    dvz_panel_set_domain(panel, DVZ_DIM_X, 0.0, 10.0);
    dvz_panel_set_domain(panel, DVZ_DIM_Y, -2.0, 2.0);

    int rc = dvz_panel_data_to_visual_positions(
        panel, (const float*)data_positions, (float*)visual_positions, PATH_COUNT);
    EXAMPLE_CHECK(rc == 0, "dvz_panel_data_to_visual_positions() failed");

    DvzVisual* path = dvz_path(scene, 0);
    EXAMPLE_CHECK(path != NULL, "dvz_path() failed");

    ok = _upload_path(path, visual_positions, colors, widths, PATH_COUNT);
    EXAMPLE_CHECK(ok, "path data upload failed");

    rc = dvz_panel_add_visual(panel, path, NULL);
    EXAMPLE_CHECK(rc == 0, "dvz_panel_add_visual() failed");

    DvzAxis* x_axis = dvz_panel_axis(panel, DVZ_DIM_X);
    DvzAxis* y_axis = dvz_panel_axis(panel, DVZ_DIM_Y);
    EXAMPLE_CHECK(x_axis != NULL && y_axis != NULL, "dvz_panel_axis() failed");

    DvzAxisTickPolicy ticks = dvz_axis_tick_policy();
    ticks.target_count = 6;
    ticks.min_pixel_spacing = 110.0f;
    ticks.minor_per_interval = 3;
    ok = dvz_axis_set_tick_policy(x_axis, &ticks);
    EXAMPLE_CHECK(ok, "dvz_axis_set_tick_policy() failed for X");
    ok = dvz_axis_set_tick_policy(y_axis, &ticks);
    EXAMPLE_CHECK(ok, "dvz_axis_set_tick_policy() failed for Y");

    ok = example_graphite_cyan_apply_axis_style(x_axis, false, NULL);
    EXAMPLE_CHECK(ok, "dvz_axis_set_style() failed for X");
    ok = example_graphite_cyan_apply_axis_style(y_axis, true, NULL);
    EXAMPLE_CHECK(ok, "dvz_axis_set_style() failed for Y");

    ok = dvz_axis_set_grid(x_axis, true);
    EXAMPLE_CHECK(ok, "dvz_axis_set_grid() failed for X");
    ok = dvz_axis_set_grid(y_axis, true);
    EXAMPLE_CHECK(ok, "dvz_axis_set_grid() failed for Y");
    ok = dvz_axis_set_label(x_axis, "time (s)");
    EXAMPLE_CHECK(ok, "dvz_axis_set_label() failed for X");
    ok = dvz_axis_set_label(y_axis, "signal");
    EXAMPLE_CHECK(ok, "dvz_axis_set_label() failed for Y");

    app = dvz_app(scene);
    EXAMPLE_CHECK(app != NULL, "dvz_app() failed (no GPU or display?)");

    DvzView* win = dvz_view_glfw(app, figure, WIDTH, HEIGHT, "path_axes_2d");
    EXAMPLE_CHECK(win != NULL, "dvz_view_glfw() failed (GLFW unavailable?)");

    DvzPanzoom* panzoom = dvz_view_panzoom(win, panel, NULL);
    EXAMPLE_CHECK(panzoom != NULL, "failed to create or bind panzoom controller");

    dvz_app_run(app, example_frame_count(argc, argv));
    ret = 0;

cleanup:
    if (app != NULL)
        dvz_app_destroy(app);
    if (scene != NULL)
        dvz_scene_destroy(scene);
    return ret;
}
