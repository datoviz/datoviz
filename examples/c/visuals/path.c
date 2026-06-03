/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* path - deterministic multi-signal retained path visual.
 *
 * Scenario: visual.path
 * Style: visuals, graphite_cyan, 1600x1200 capture target
 *
 * Build:  just example-c visuals/path
 * Run:    ./build/examples/c/visuals/path
 * Smoke:  ./build/examples/c/visuals/path 1
 * PNG:    DVZ_CAPTURE=png ./build/examples/c/visuals/path 1
 */



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "_assertions.h"
#include "datoviz/app.h"
#include "datoviz/scene.h"
#include "example_common.h"
#include "example_style.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH            1600u
#define HEIGHT           1200u
#define PATH_COUNT       3u
#define SAMPLES_PER_PATH 192u
#define SAMPLE_COUNT     (PATH_COUNT * SAMPLES_PER_PATH)

static const float TAU = 6.28318530718f;



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Fill deterministic continuous polyline samples.
 *
 * @param positions output visual-space path positions
 * @param colors output per-sample path colors
 * @param widths output per-sample stroke widths
 * @param subpaths output path lengths
 */
static void _fill_paths(
    vec3 positions[SAMPLE_COUNT], DvzColor colors[SAMPLE_COUNT], float widths[SAMPLE_COUNT],
    uint32_t subpaths[PATH_COUNT])
{
    ANN(positions);
    ANN(colors);
    ANN(widths);
    ANN(subpaths);

    const DvzColor palette[PATH_COUNT] = {
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ACCENT_PRIMARY),
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ACCENT_SECONDARY),
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_WARNING),
    };
    const float lanes[PATH_COUNT] = {+0.42f, 0.0f, -0.42f};
    const float width_base[PATH_COUNT] = {2.0f, 7.0f, 16.0f};
    const float width_amp[PATH_COUNT] = {0.8f, 2.2f, 6.0f};

    for (uint32_t path = 0; path < PATH_COUNT; path++)
    {
        subpaths[path] = SAMPLES_PER_PATH;
        const float phase = 0.55f * (float)path;
        const float amp = 0.115f + 0.025f * (float)path;
        const float frequency = 1.35f + 0.45f * (float)path;

        for (uint32_t i = 0; i < SAMPLES_PER_PATH; i++)
        {
            const uint32_t k = path * SAMPLES_PER_PATH + i;
            const float t =
                SAMPLES_PER_PATH > 1u ? (float)i / (float)(SAMPLES_PER_PATH - 1u) : 0.0f;
            const float x = -0.86f + 1.72f * t;
            const float envelope = 0.72f + 0.28f * sinf(TAU * t);
            const float carrier = sinf(TAU * (frequency * t + 0.07f * sinf(TAU * t)) + phase);
            const float detail = 0.32f * sinf(TAU * ((frequency + 2.15f) * t) + 1.4f * phase);
            const float y = lanes[path] + amp * envelope * (carrier + detail);

            positions[k][0] = x;
            positions[k][1] = y;
            positions[k][2] = 0.0f;

            colors[k] = palette[path];
            colors[k].a = (uint8_t)(210u + 18u * path);
            const float width_wave = 0.5f + 0.5f * sinf(TAU * t + phase);
            widths[k] = width_base[path] + width_amp[path] * width_wave;
        }
    }
}



/**
 * Add the path visual to the panel.
 *
 * @param scene scene owning the visual
 * @param panel panel receiving the visual
 * @param positions visual-space path positions
 * @param colors per-sample colors
 * @param widths per-sample stroke widths
 * @param subpaths path lengths
 * @return true when the path was added
 */
static bool _add_path(
    DvzScene* scene, DvzPanel* panel, vec3 positions[SAMPLE_COUNT], DvzColor colors[SAMPLE_COUNT],
    float widths[SAMPLE_COUNT], uint32_t subpaths[PATH_COUNT])
{
    ANN(scene);
    ANN(panel);
    ANN(positions);
    ANN(colors);
    ANN(widths);
    ANN(subpaths);

    DvzVisual* path = dvz_path(scene, 0);
    if (path == NULL)
        return false;

    DvzVisualDataUpdate updates[] = {
        {.attr_name = "position", .data = positions, .item_count = SAMPLE_COUNT},
        {.attr_name = "color", .data = colors, .item_count = SAMPLE_COUNT},
        {.attr_name = "stroke_width", .data = widths, .item_count = SAMPLE_COUNT},
    };
    if (dvz_visual_set_data_many(path, updates, 3) != 0)
        return false;
    if (dvz_path_set_subpaths(path, PATH_COUNT, subpaths) != 0)
        return false;
    if (dvz_path_set_caps(path, DVZ_SEGMENT_CAP_ROUND, DVZ_SEGMENT_CAP_ROUND) != 0)
        return false;
    if (dvz_path_set_join(path, DVZ_PATH_JOIN_ROUND, 4.0f) != 0)
        return false;
    if (dvz_visual_set_alpha_mode(path, DVZ_ALPHA_BLENDED) != 0)
        return false;
    if (dvz_visual_set_depth_test(path, false) != 0)
        return false;
    return dvz_panel_add_visual(panel, path, NULL) == 0;
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Run the retained path visual example.
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
    vec3 positions[SAMPLE_COUNT] = {{0}};
    DvzColor colors[SAMPLE_COUNT] = {{0}};
    float widths[SAMPLE_COUNT] = {0};
    uint32_t subpaths[PATH_COUNT] = {0};
    const uint32_t frame_count = example_frame_count_any(argc, argv);
    DvzAppCaptureConfig capture = dvz_app_capture_config_from_env("visual_path");

    _fill_paths(positions, colors, widths, subpaths);

    scene = dvz_scene();
    EXAMPLE_CHECK(scene != NULL, "dvz_scene() failed");

    DvzFigure* figure = dvz_figure(scene, WIDTH, HEIGHT, 0);
    EXAMPLE_CHECK(figure != NULL, "dvz_figure() failed");

    DvzPanel* panel = dvz_panel_full(figure);
    EXAMPLE_CHECK(panel != NULL, "dvz_panel_full() failed");
    example_graphite_cyan_set_panel_background(panel);

    bool ok = _add_path(scene, panel, positions, colors, widths, subpaths);
    EXAMPLE_CHECK(ok, "adding path visual failed");

    app = dvz_app(scene);
    EXAMPLE_CHECK(app != NULL, "dvz_app() failed (no GPU or display?)");

    win = dvz_view_glfw(app, figure, WIDTH, HEIGHT, "visual_path");
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
