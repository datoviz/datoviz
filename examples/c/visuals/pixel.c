/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* pixel - deterministic retained pixel visual baseline.
 *
 * Scenario: visual.pixel
 * Style: visuals, graphite_cyan, 1600x1200 capture target
 *
 * Build:  just example-c visuals/pixel
 * Run:    ./build/examples/c/visuals/pixel
 * Smoke:  ./build/examples/c/visuals/pixel 1
 * PNG:    DVZ_CAPTURE=png ./build/examples/c/visuals/pixel 1
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
#define GRID_WIDTH  72u
#define GRID_HEIGHT 54u
#define PIXEL_COUNT (GRID_WIDTH * GRID_HEIGHT)

static const float TAU = 6.28318530718f;



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Return one deterministic scalar sample for the pixel grid.
 *
 * @param u normalized grid x coordinate
 * @param v normalized grid y coordinate
 * @return normalized scalar value
 */
static float _sample_value(float u, float v)
{
    const float ridge = 0.5f + 0.5f * sinf(TAU * (1.7f * u + 0.35f * v));
    const float wave = 0.5f + 0.5f * cosf(TAU * (0.6f * u - 1.9f * v));
    const float dx = u - 0.34f;
    const float dy = v - 0.64f;
    const float blob = expf(-(dx * dx + 1.6f * dy * dy) / 0.012f);
    const float value = 0.08f + 0.36f * u + 0.18f * v + 0.16f * ridge + 0.10f * wave +
                        0.28f * blob;
    return fminf(1.0f, fmaxf(0.0f, value));
}



/**
 * Fill the deterministic pixel grid.
 *
 * @param positions output pixel positions
 * @param values output scalar values
 * @param sizes output pixel sprite sizes in pixels
 */
static void _fill_pixels(
    vec3 positions[PIXEL_COUNT], float values[PIXEL_COUNT], float sizes[PIXEL_COUNT])
{
    ANN(positions);
    ANN(values);
    ANN(sizes);

    const float step_x = 1.82f / (float)(GRID_WIDTH - 1u);
    const float step_y = 1.36f / (float)(GRID_HEIGHT - 1u);

    for (uint32_t y = 0; y < GRID_HEIGHT; y++)
    {
        for (uint32_t x = 0; x < GRID_WIDTH; x++)
        {
            const uint32_t i = y * GRID_WIDTH + x;
            const float u = (float)x / (float)(GRID_WIDTH - 1u);
            const float v = (float)y / (float)(GRID_HEIGHT - 1u);
            const float value = _sample_value(u, v);

            positions[i][0] = -0.91f + step_x * (float)x;
            positions[i][1] = -0.68f + step_y * (float)y;
            positions[i][2] = 0.0f;

            values[i] = value;
            sizes[i] = 9.0f + 4.0f * value;
        }
    }
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Run the retained pixel visual example.
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
    float* sizes = NULL;
    const uint32_t frame_count = example_frame_count_any(argc, argv);
    DvzAppCaptureConfig capture = dvz_app_capture_config_from_env("visual_pixel");

    positions = (vec3*)dvz_calloc(PIXEL_COUNT, sizeof(*positions));
    values = (float*)dvz_calloc(PIXEL_COUNT, sizeof(*values));
    sizes = (float*)dvz_calloc(PIXEL_COUNT, sizeof(*sizes));
    EXAMPLE_CHECK(positions != NULL && values != NULL && sizes != NULL, "pixel allocation failed");

    _fill_pixels(positions, values, sizes);

    scene = dvz_scene();
    EXAMPLE_CHECK(scene != NULL, "dvz_scene() failed");

    DvzFigure* figure = dvz_figure(scene, WIDTH, HEIGHT, 0);
    EXAMPLE_CHECK(figure != NULL, "dvz_figure() failed");

    DvzPanel* panel = dvz_panel_full(figure);
    EXAMPLE_CHECK(panel != NULL, "dvz_panel_full() failed");
    example_graphite_cyan_set_panel_background(panel);

    DvzVisual* visual = dvz_pixel(scene, 0);
    EXAMPLE_CHECK(visual != NULL, "dvz_pixel() failed");

    int rc = dvz_visual_set_attr_format(visual, "color", DVZ_VISUAL_ATTR_FORMAT_SCALAR_F32);
    EXAMPLE_CHECK(rc == 0, "dvz_visual_set_attr_format() failed");

    DvzScale* color_scale = example_graphite_cyan_color_scale(scene, 0.0, 1.0);
    EXAMPLE_CHECK(color_scale != NULL, "example_graphite_cyan_color_scale() failed");

    rc = dvz_visual_set_scale(visual, "color", color_scale);
    EXAMPLE_CHECK(rc == 0, "dvz_visual_set_scale() failed");

    DvzVisualDataUpdate updates[] = {
        {.attr_name = "position", .data = positions, .item_count = PIXEL_COUNT},
        {.attr_name = "color", .data = values, .item_count = PIXEL_COUNT},
        {.attr_name = "pixel_size", .data = sizes, .item_count = PIXEL_COUNT},
    };
    rc = dvz_visual_set_data_many(visual, updates, 3);
    EXAMPLE_CHECK(rc == 0, "pixel data upload failed");

    rc = dvz_visual_set_depth_test(visual, false);
    EXAMPLE_CHECK(rc == 0, "dvz_visual_set_depth_test() failed");

    rc = dvz_panel_add_visual(panel, visual, NULL);
    EXAMPLE_CHECK(rc == 0, "dvz_panel_add_visual() failed");

    app = dvz_app(scene);
    EXAMPLE_CHECK(app != NULL, "dvz_app() failed (no GPU or display?)");

    win = dvz_view_glfw(app, figure, WIDTH, HEIGHT, "visual_pixel");
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
    dvz_free(sizes);
    dvz_free(values);
    dvz_free(positions);
    if (scene != NULL)
        dvz_scene_destroy(scene);
    return ret;
}
