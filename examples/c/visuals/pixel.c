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
 * Clamp a float to the unit interval.
 *
 * @param value input value
 * @return clamped value
 */
static float _clamp01(float value)
{
    if (value < 0.0f)
        return 0.0f;
    if (value > 1.0f)
        return 1.0f;
    return value;
}



/**
 * Convert a normalized scalar to an 8-bit channel.
 *
 * @param value normalized value
 * @return clamped 8-bit channel
 */
static uint8_t _u8(float value) { return (uint8_t)(255.0f * _clamp01(value) + 0.5f); }



/**
 * Return one deterministic scalar sample for the pixel grid.
 *
 * @param u normalized grid x coordinate
 * @param v normalized grid y coordinate
 * @return normalized scalar value
 */
static float _sample_value(float u, float v)
{
    float value = 0.16f + 0.42f * u + 0.24f * v;
    value += 0.12f * sinf(TAU * (2.2f * u + 0.4f * v));
    value += 0.10f * cosf(TAU * (0.7f * u - 2.8f * v));

    const float dx0 = u - 0.30f;
    const float dy0 = v - 0.68f;
    const float dx1 = u - 0.72f;
    const float dy1 = v - 0.35f;
    value += 0.28f * expf(-(dx0 * dx0 + 1.7f * dy0 * dy0) / (2.0f * 0.055f * 0.055f));
    value -= 0.20f * expf(-(1.4f * dx1 * dx1 + dy1 * dy1) / (2.0f * 0.075f * 0.075f));

    return _clamp01(value);
}



/**
 * Fill the deterministic pixel grid.
 *
 * @param positions output pixel positions
 * @param colors output pixel colors
 * @param sizes output pixel sprite sizes in pixels
 */
static void
_fill_pixels(vec3 positions[PIXEL_COUNT], DvzColor colors[PIXEL_COUNT], float sizes[PIXEL_COUNT])
{
    ANN(positions);
    ANN(colors);
    ANN(sizes);

    const DvzColor cyan = example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ACCENT_PRIMARY);
    const DvzColor mint = example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ACCENT_SECONDARY);
    const DvzColor warm = example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_WARNING);
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
            const float checker = (float)((x + y) & 1u);

            positions[i][0] = -0.91f + step_x * (float)x;
            positions[i][1] = -0.68f + step_y * (float)y;
            positions[i][2] = 0.0f;

            colors[i] = dvz_color_rgba(
                _u8(((1.0f - value) * cyan.r + value * mint.r) / 255.0f +
                    0.16f * value * warm.r / 255.0f),
                _u8(((1.0f - value) * cyan.g + value * mint.g) / 255.0f),
                _u8(((1.0f - value) * cyan.b + value * mint.b) / 255.0f - 0.08f * checker), 252);
            sizes[i] = 9.0f + 4.0f * value;
        }
    }
}



/**
 * Upload the pixel arrays to one retained visual.
 *
 * @param visual pixel visual
 * @param positions pixel positions
 * @param colors pixel colors
 * @param sizes pixel sizes
 * @return true when all uploads succeed
 */
static bool _upload_pixels(
    DvzVisual* visual, vec3 positions[PIXEL_COUNT], DvzColor colors[PIXEL_COUNT],
    float sizes[PIXEL_COUNT])
{
    ANN(visual);
    ANN(positions);
    ANN(colors);
    ANN(sizes);

    DvzVisualDataUpdate updates[] = {
        {.attr_name = "position", .data = positions, .item_count = PIXEL_COUNT},
        {.attr_name = "color", .data = colors, .item_count = PIXEL_COUNT},
        {.attr_name = "pixel_size", .data = sizes, .item_count = PIXEL_COUNT},
    };
    return dvz_visual_set_data_many(visual, updates, 3) == 0;
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
    DvzColor* colors = NULL;
    float* sizes = NULL;
    const uint32_t frame_count = example_frame_count_any(argc, argv);
    DvzAppCaptureConfig capture = dvz_app_capture_config_from_env("visual_pixel");

    positions = (vec3*)dvz_calloc(PIXEL_COUNT, sizeof(*positions));
    colors = (DvzColor*)dvz_calloc(PIXEL_COUNT, sizeof(*colors));
    sizes = (float*)dvz_calloc(PIXEL_COUNT, sizeof(*sizes));
    EXAMPLE_CHECK(positions != NULL && colors != NULL && sizes != NULL, "pixel allocation failed");

    _fill_pixels(positions, colors, sizes);

    scene = dvz_scene();
    EXAMPLE_CHECK(scene != NULL, "dvz_scene() failed");

    DvzFigure* figure = dvz_figure(scene, WIDTH, HEIGHT, 0);
    EXAMPLE_CHECK(figure != NULL, "dvz_figure() failed");

    DvzPanel* panel = dvz_panel_full(figure);
    EXAMPLE_CHECK(panel != NULL, "dvz_panel_full() failed");
    example_graphite_cyan_set_panel_background(panel);

    DvzVisual* visual = dvz_pixel(scene, 0);
    EXAMPLE_CHECK(visual != NULL, "dvz_pixel() failed");

    bool ok = _upload_pixels(visual, positions, colors, sizes);
    EXAMPLE_CHECK(ok, "pixel data upload failed");

    int rc = dvz_visual_set_depth_test(visual, false);
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
    dvz_free(colors);
    dvz_free(positions);
    if (scene != NULL)
        dvz_scene_destroy(scene);
    return ret;
}
