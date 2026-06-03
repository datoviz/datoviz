/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* splat - retained Gaussian splat visual with deterministic screen-space ellipses.
 *
 * Scenario: visual.splat
 * Style: visuals, graphite_cyan, 1600x1200 capture target
 *
 * Build:  just example-c visuals/splat
 * Run:    ./build/examples/c/visuals/splat
 * Smoke:  ./build/examples/c/visuals/splat 1
 * PNG:    DVZ_CAPTURE=png ./build/examples/c/visuals/splat 1
 */



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <math.h>
#include <stdbool.h>
#include <stdint.h>

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
#define SPLAT_COUNT 84u

static const float TAU = 6.28318530718f;



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Fill a small deterministic Gaussian splat cloud.
 *
 * @param positions output splat centers
 * @param colors output splat colors
 * @param sigma output screen-space ellipse radii in pixels
 * @param angles output ellipse angles in radians
 */
static void _fill_splats(
    vec3 positions[SPLAT_COUNT], DvzColor colors[SPLAT_COUNT], vec2 sigma[SPLAT_COUNT],
    float angles[SPLAT_COUNT])
{
    ANN(positions);
    ANN(colors);
    ANN(sigma);
    ANN(angles);

    const ExampleStyleColorRole roles[] = {
        EXAMPLE_STYLE_COLOR_ACCENT_PRIMARY,
        EXAMPLE_STYLE_COLOR_ACCENT_SECONDARY,
        EXAMPLE_STYLE_COLOR_WARNING,
        EXAMPLE_STYLE_COLOR_ERROR,
    };

    for (uint32_t i = 0; i < SPLAT_COUNT; i++)
    {
        const float t = SPLAT_COUNT > 1u ? (float)i / (float)(SPLAT_COUNT - 1u) : 0.0f;
        const float arm = (float)(i % 7u);
        const float local = (float)(i / 7u) / (float)(SPLAT_COUNT / 7u);
        const float theta = TAU * (1.55f * local + arm / 7.0f);
        const float radius = 0.10f + 0.78f * sqrtf(local);

        positions[i][0] = radius * cosf(theta);
        positions[i][1] = 0.76f * radius * sinf(theta);
        positions[i][2] = 0.01f * sinf(TAU * t);

        colors[i] = example_graphite_cyan_color(roles[i % DVZ_ARRAY_COUNT(roles)]);
        colors[i].a = 118u + (uint8_t)(64u * (i % 3u));

        sigma[i][0] = 10.0f + 7.5f * (0.5f + 0.5f * sinf(TAU * (t + 0.11f * arm)));
        sigma[i][1] = 4.0f + 4.5f * (0.5f + 0.5f * cosf(TAU * (0.7f * t + 0.05f * arm)));
        angles[i] = theta + 0.45f * sinf(TAU * t);
    }
}



/**
 * Add one retained Gaussian splat visual to the panel.
 *
 * @param scene scene owning the visual
 * @param panel panel receiving the visual
 * @return true when the visual was added
 */
static bool _add_splats(DvzScene* scene, DvzPanel* panel)
{
    ANN(scene);
    ANN(panel);

    vec3 positions[SPLAT_COUNT] = {{0}};
    DvzColor colors[SPLAT_COUNT] = {{0}};
    vec2 sigma[SPLAT_COUNT] = {{0}};
    float angles[SPLAT_COUNT] = {0};
    _fill_splats(positions, colors, sigma, angles);

    DvzVisual* visual = dvz_splat(scene, 0);
    if (visual == NULL)
        return false;

    DvzVisualDataUpdate updates[] = {
        {.attr_name = "position", .data = positions, .item_count = SPLAT_COUNT},
        {.attr_name = "color", .data = colors, .item_count = SPLAT_COUNT},
        {.attr_name = "sigma", .data = sigma, .item_count = SPLAT_COUNT},
        {.attr_name = "angle", .data = angles, .item_count = SPLAT_COUNT},
    };
    if (dvz_visual_set_data_many(visual, updates, 4) != 0)
        return false;
    return dvz_panel_add_visual(panel, visual, NULL) == 0;
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Run the retained Gaussian splat visual example.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @return process exit code
 */
int main(int argc, char** argv)
{
    const uint32_t frame_count = example_frame_count_any(argc, argv);
    DvzAppCaptureConfig capture = dvz_app_capture_config_from_env("visual_splat");

    int ret = 1;
    DvzScene* scene = NULL;
    DvzApp* app = NULL;
    DvzView* win = NULL;
    bool capture_started = false;

    scene = dvz_scene();
    EXAMPLE_CHECK(scene != NULL, "dvz_scene() failed");

    DvzFigure* figure = dvz_figure(scene, WIDTH, HEIGHT, 0);
    EXAMPLE_CHECK(figure != NULL, "dvz_figure() failed");

    DvzPanel* panel = dvz_panel_full(figure);
    EXAMPLE_CHECK(panel != NULL, "dvz_panel_full() failed");
    example_graphite_cyan_set_panel_background(panel);

    EXAMPLE_CHECK(_add_splats(scene, panel), "splat visual setup failed");

    app = dvz_app(scene);
    EXAMPLE_CHECK(app != NULL, "dvz_app() failed (no GPU or display?)");

    win = dvz_view_glfw(app, figure, WIDTH, HEIGHT, "visual_splat");
    EXAMPLE_CHECK(win != NULL, "dvz_view_glfw() failed (GLFW unavailable?)");

    DvzPanzoom* panzoom = dvz_view_panzoom(win, panel, NULL);
    EXAMPLE_CHECK(panzoom != NULL, "failed to create or bind panzoom controller");

    int rc = dvz_view_capture_start(win, &capture);
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
    if (scene != NULL)
        dvz_scene_destroy(scene);
    return ret;
}
