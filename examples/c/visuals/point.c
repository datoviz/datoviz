/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* point - deterministic retained 2D point visual baseline.
 *
 * Scenario: visual.point / point_2d
 * Style: visuals, graphite_cyan, 1600x1200 capture target
 *
 * Build:  just example-c visuals/point
 * Run:    ./build/examples/c/visuals/point
 * Smoke:  ./build/examples/c/visuals/point 1
 * PNG:    DVZ_CAPTURE=png ./build/examples/c/visuals/point 1
 */



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <math.h>
#include <stdbool.h>
#include <stdint.h>

#include "_alloc.h"
#include "_assertions.h"
#include "datoviz/app.h"
#include "datoviz/scene.h"
#include "example_common.h"
#include "example_style.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH       1600u
#define HEIGHT      1200u
#define POINT_COUNT 960u

static const float TAU = 6.28318530718f;



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Fill one deterministic compact point cloud.
 *
 * @param positions output point positions
 * @param values output point scalar values
 * @param diameters output point diameters in pixels
 */
static void _fill_points(
    vec3 positions[POINT_COUNT], float values[POINT_COUNT], float diameters[POINT_COUNT])
{
    ANN(positions);
    ANN(values);
    ANN(diameters);

    for (uint32_t i = 0; i < POINT_COUNT; i++)
    {
        const float t = POINT_COUNT > 1u ? (float)i / (float)(POINT_COUNT - 1u) : 0.0f;
        const float arm = (float)(i % 6u);
        const float local = (float)(i / 6u) / (float)(POINT_COUNT / 6u);
        const float theta = TAU * (2.10f * local + arm / 6.0f);
        const float radius = 0.10f + 0.82f * sqrtf(local);
        const float ripple = 0.040f * sinf(TAU * (3.0f * local + 0.13f * arm));

        positions[i][0] = (radius + ripple) * cosf(theta);
        positions[i][1] = 0.84f * (radius - 0.5f * ripple) * sinf(theta);
        positions[i][2] = 0.0f;

        const float band = 0.5f + 0.5f * sinf(TAU * (t + 0.08f * arm));
        const float mix = 0.25f + 0.75f * sqrtf(local);
        values[i] = fminf(1.0f, 0.12f + 0.76f * mix + 0.12f * band);

        diameters[i] = 10.0f + 11.0f * band + 5.0f * (1.0f - local);
    }
}



/**
 * Upload the point arrays to one retained visual.
 *
 * @param visual point visual
 * @param positions point positions
 * @param values point scalar values
 * @param diameters point diameters
 * @return true when all uploads succeed
 */
static bool _upload_points(
    DvzVisual* visual, vec3 positions[POINT_COUNT], float values[POINT_COUNT],
    float diameters[POINT_COUNT])
{
    ANN(visual);
    ANN(positions);
    ANN(values);
    ANN(diameters);

    DvzVisualDataUpdate updates[] = {
        {.attr_name = "position", .data = positions, .item_count = POINT_COUNT},
        {.attr_name = "color", .data = values, .item_count = POINT_COUNT},
        {.attr_name = "diameter", .data = diameters, .item_count = POINT_COUNT},
    };
    return dvz_visual_set_data_many(visual, updates, 3) == 0;
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Run the retained 2D point visual example.
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
    DvzView* win = NULL;
    bool capture_started = false;
    vec3* positions = NULL;
    float* values = NULL;
    float* diameters = NULL;
    const uint32_t frame_count = example_frame_count_any(argc, argv);
    DvzAppCaptureConfig capture = dvz_app_capture_config_from_env("visual_point");

    positions = (vec3*)dvz_calloc(POINT_COUNT, sizeof(*positions));
    values = (float*)dvz_calloc(POINT_COUNT, sizeof(*values));
    diameters = (float*)dvz_calloc(POINT_COUNT, sizeof(*diameters));
    EXAMPLE_CHECK(
        positions != NULL && values != NULL && diameters != NULL, "point allocation failed");

    _fill_points(positions, values, diameters);

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

    bool ok = _upload_points(point, positions, values, diameters);
    EXAMPLE_CHECK(ok, "point data upload failed");

    DvzPointStyleDesc style = dvz_point_style_desc();
    style.aspect = DVZ_SHAPE_ASPECT_FILLED;
    style.stroke_width = 0.0f;
    rc = dvz_point_set_style(point, &style);
    EXAMPLE_CHECK(rc == 0, "dvz_point_set_style() failed");

    rc = dvz_visual_set_depth_test(point, false);
    EXAMPLE_CHECK(rc == 0, "dvz_visual_set_depth_test() failed");

    rc = dvz_visual_set_alpha_mode(point, DVZ_ALPHA_BLENDED);
    EXAMPLE_CHECK(rc == 0, "dvz_visual_set_alpha_mode() failed");

    rc = dvz_panel_add_visual(panel, point, NULL);
    EXAMPLE_CHECK(rc == 0, "dvz_panel_add_visual() failed");

    app = dvz_app(scene);
    EXAMPLE_CHECK(app != NULL, "dvz_app() failed (no GPU or display?)");

    win = dvz_view_glfw(app, figure, WIDTH, HEIGHT, "visual_point");
    EXAMPLE_CHECK(win != NULL, "dvz_view_glfw() failed (GLFW unavailable?)");

    DvzPanzoom* panzoom = dvz_view_panzoom(win, panel, NULL);
    EXAMPLE_CHECK(panzoom != NULL, "failed to create or bind panzoom controller");

    rc = dvz_view_capture_start(win, &capture);
    EXAMPLE_CHECK(rc == 0, "dvz_view_capture_start() failed");
    capture_started = true;

    dvz_app_run(app, frame_count);

    rc = dvz_view_capture_stop(win);
    EXAMPLE_CHECK(rc == 0, "dvz_view_capture_stop() failed");
    capture_started = false;
    ret = 0;

cleanup:
    if (capture_started && win != NULL)
        (void)dvz_view_capture_stop(win);
    if (app != NULL)
        dvz_app_destroy(app);
    dvz_free(diameters);
    dvz_free(values);
    dvz_free(positions);
    if (scene != NULL)
        dvz_scene_destroy(scene);
    return ret;
}
