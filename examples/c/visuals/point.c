/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* point - minimal retained 2D point visual baseline.
 *
 * Scenario: point_2d
 * Style: visuals, graphite_cyan, 1280x960 capture target
 *
 * Build:  just example-c visuals/point
 * Run:    ./build/examples/c/visuals/point
 * Smoke:  ./build/examples/c/visuals/point 1
 */



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_compat.h"
#include "datoviz/app.h"
#include "datoviz/scene.h"
#include "example_common.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH       1280u
#define HEIGHT      960u
#define POINT_COUNT 960u

static const float TAU = 6.28318530718f;



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Fill a deterministic 2D point cloud for the visual baseline.
 *
 * @param positions point positions
 * @param colors point colors
 * @param diameters point diameters
 * @param count point count
 */
static void _fill_points(vec3* positions, DvzColor* colors, float* diameters, uint32_t count)
{
    ANN(positions);
    ANN(colors);
    ANN(diameters);

    const float inv_count = count > 1 ? 1.0f / (float)(count - 1u) : 1.0f;
    for (uint32_t i = 0; i < count; i++)
    {
        const float t = (float)i * inv_count;
        const float arm = (float)(i % 5u);
        const float local = (float)(i / 5u) / (float)(count / 5u);
        const float theta = TAU * (local * 2.15f + arm / 5.0f);
        const float radius = 0.10f + 0.86f * sqrtf(local);
        const float wave = 0.045f * sinf(12.0f * local + 1.7f * arm);

        positions[i][0] = (radius + wave) * cosf(theta);
        positions[i][1] = (radius - 0.5f * wave) * sinf(theta);
        positions[i][2] = 0.0f;

        const float cyan = 0.35f + 0.65f * t;
        const float warm = 0.5f + 0.5f * sinf(TAU * t);
        const uint8_t r = (uint8_t)(46.0f + 120.0f * warm);
        const uint8_t g = (uint8_t)(150.0f + 85.0f * cyan);
        const uint8_t b = (uint8_t)(175.0f + 60.0f * (1.0f - warm));
        colors[i] = dvz_color_rgba(r, g, b, 245);

        diameters[i] = 3.8f + 2.8f * (0.5f + 0.5f * sinf(TAU * (3.0f * t + 0.17f)));
    }
}



/**
 * Upload the point baseline arrays to a retained visual.
 *
 * @param visual point visual
 * @param positions point positions
 * @param colors point colors
 * @param diameters point diameters
 * @param count point count
 * @return true when all uploads succeed
 */
static bool _upload_points(
    DvzVisual* visual, vec3* positions, DvzColor* colors, float* diameters, uint32_t count)
{
    ANN(visual);
    ANN(positions);
    ANN(colors);
    ANN(diameters);

    DvzVisualDataUpdate updates[] = {
        {.attr_name = "position", .data = positions, .item_count = count},
        {.attr_name = "color", .data = colors, .item_count = count},
        {.attr_name = "diameter", .data = diameters, .item_count = count},
    };
    return dvz_visual_set_data_many(visual, updates, 3) == 0;
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Run the minimal 2D point visual baseline.
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
    vec3* positions = NULL;
    DvzColor* colors = NULL;
    float* diameters = NULL;

    if (POINT_COUNT > SIZE_MAX / sizeof(*positions) ||
        POINT_COUNT > SIZE_MAX / sizeof(*colors) || POINT_COUNT > SIZE_MAX / sizeof(*diameters))
    {
        dvz_fprintf(stderr, "point allocation size overflow\n");
        goto cleanup;
    }

    positions = (vec3*)dvz_calloc(POINT_COUNT, sizeof(*positions));
    colors = (DvzColor*)dvz_calloc(POINT_COUNT, sizeof(*colors));
    diameters = (float*)dvz_calloc(POINT_COUNT, sizeof(*diameters));
    EXAMPLE_CHECK(
        positions != NULL && colors != NULL && diameters != NULL, "point allocation failed");

    _fill_points(positions, colors, diameters, POINT_COUNT);

    scene = dvz_scene();
    EXAMPLE_CHECK(scene != NULL, "dvz_scene() failed");

    DvzFigure* figure = dvz_figure(scene, WIDTH, HEIGHT, 0);
    EXAMPLE_CHECK(figure != NULL, "dvz_figure() failed");

    DvzPanel* panel = dvz_panel_full(figure);
    EXAMPLE_CHECK(panel != NULL, "dvz_panel_full() failed");
    dvz_panel_set_background_color(panel, 0.055f, 0.067f, 0.090f, 1.0f);

    DvzVisual* visual = dvz_point(scene, 0);
    EXAMPLE_CHECK(visual != NULL, "dvz_point() failed");

    bool ok = _upload_points(visual, positions, colors, diameters, POINT_COUNT);
    EXAMPLE_CHECK(ok, "point data upload failed");

    DvzPointStyleDesc style = dvz_point_style_desc();
    style.aspect = DVZ_SHAPE_ASPECT_FILLED;
    style.stroke_width = 0.0f;
    ok = dvz_point_set_style(visual, &style) == 0;
    EXAMPLE_CHECK(ok, "dvz_point_set_style() failed");

    int rc = dvz_visual_set_depth_test(visual, false);
    EXAMPLE_CHECK(rc == 0, "dvz_visual_set_depth_test() failed");

    rc = dvz_panel_add_visual(panel, visual, NULL);
    EXAMPLE_CHECK(rc == 0, "dvz_panel_add_visual() failed");

    app = dvz_app(scene);
    EXAMPLE_CHECK(app != NULL, "dvz_app() failed (no GPU or display?)");

    DvzView* win = dvz_view_glfw(app, figure, WIDTH, HEIGHT, "point_2d");
    EXAMPLE_CHECK(win != NULL, "dvz_view_glfw() failed (GLFW unavailable?)");

    DvzPanzoom* panzoom = dvz_view_panzoom(win, panel, NULL);
    EXAMPLE_CHECK(panzoom != NULL, "failed to create or bind panzoom controller");

    dvz_app_run(app, example_frame_count(argc, argv));
    ret = 0;

cleanup:
    if (app != NULL)
        dvz_app_destroy(app);
    dvz_free(diameters);
    dvz_free(colors);
    dvz_free(positions);
    if (scene != NULL)
        dvz_scene_destroy(scene);
    return ret;
}
