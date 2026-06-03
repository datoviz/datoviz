/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* primitive - topology-parametric triangles rendered with the retained primitive visual.
 *
 * Scenario: visual.primitive
 * Style: visuals, graphite_cyan, 1600x1200 capture target
 *
 * Build:  just example-c visuals/primitive
 * Run:    ./build/examples/c/visuals/primitive
 * Smoke:  ./build/examples/c/visuals/primitive 1
 * PNG:    DVZ_CAPTURE=png ./build/examples/c/visuals/primitive 1
 */



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

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

#define WIDTH  1600u
#define HEIGHT 1200u



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Upload positions, colors, and flat normals into one primitive visual.
 *
 * @param visual primitive visual
 * @param positions vertex positions
 * @param colors vertex colors
 * @param vertex_count vertex count
 * @return true when upload succeeds
 */
static bool _upload_primitive(
    DvzVisual* visual, const vec3* positions, const DvzColor* colors, uint32_t vertex_count)
{
    ANN(visual);
    ANN(positions);
    ANN(colors);

    vec3 normals[8] = {{0}};
    ASSERT(vertex_count <= 8u);
    for (uint32_t i = 0; i < vertex_count; i++)
        normals[i][2] = 1.0f;

    DvzVisualDataUpdate updates[] = {
        {.attr_name = "position", .data = positions, .item_count = vertex_count},
        {.attr_name = "color", .data = colors, .item_count = vertex_count},
        {.attr_name = "normal", .data = normals, .item_count = vertex_count},
    };
    return dvz_visual_set_data_many(visual, updates, 3) == 0;
}



/**
 * Add one primitive topology sample to the panel.
 *
 * @param scene scene owning the visual
 * @param panel panel receiving the visual
 * @param topology primitive topology
 * @param positions vertex positions
 * @param colors vertex colors
 * @param vertex_count vertex count
 * @return true when the visual was added
 */
static bool _add_primitive(
    DvzScene* scene, DvzPanel* panel, DvzPrimitiveTopology topology, const vec3* positions,
    const DvzColor* colors, uint32_t vertex_count)
{
    ANN(scene);
    ANN(panel);

    DvzVisual* visual = dvz_primitive(scene, topology, 0);
    if (visual == NULL)
        return false;
    if (!_upload_primitive(visual, positions, colors, vertex_count))
        return false;
    return dvz_panel_add_visual(panel, visual, NULL) == 0;
}



/**
 * Add three topology-parametric primitive visuals.
 *
 * @param scene scene owning visuals
 * @param panel target panel
 * @return true when all visuals were added
 */
static bool _add_primitives(DvzScene* scene, DvzPanel* panel)
{
    ANN(scene);
    ANN(panel);

    DvzColor cyan = example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ACCENT_PRIMARY);
    DvzColor mint = example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ACCENT_SECONDARY);
    DvzColor warm = example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_WARNING);
    DvzColor rose = example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ERROR);
    DvzColor text = example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_TEXT);

    const vec3 list_positions[6] = {
        {-0.88f, -0.42f, 0.00f}, {-0.34f, -0.42f, 0.00f}, {-0.61f, +0.44f, 0.00f},
        {-0.96f, +0.10f, 0.03f}, {-0.47f, +0.58f, 0.03f}, {-0.20f, -0.06f, 0.03f},
    };
    const DvzColor list_colors[6] = {cyan, mint, warm, rose, text, cyan};
    if (!_add_primitive(
            scene, panel, DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, list_positions, list_colors, 6))
        return false;

    const vec3 strip_positions[6] = {
        {-0.12f, +0.08f, 0.00f}, {+0.10f, +0.74f, 0.00f}, {+0.28f, +0.18f, 0.02f},
        {+0.48f, +0.82f, 0.02f}, {+0.66f, +0.26f, 0.04f}, {+0.88f, +0.70f, 0.04f},
    };
    const DvzColor strip_colors[6] = {rose, cyan, mint, warm, cyan, text};
    if (!_add_primitive(
            scene, panel, DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP, strip_positions, strip_colors, 6))
        return false;

    const vec3 fan_positions[8] = {
        {+0.38f, -0.44f, 0.06f}, {+0.16f, -0.86f, 0.06f}, {+0.54f, -0.92f, 0.06f},
        {+0.88f, -0.70f, 0.06f}, {+0.92f, -0.34f, 0.06f}, {+0.70f, -0.02f, 0.06f},
        {+0.34f, +0.04f, 0.06f}, {+0.08f, -0.18f, 0.06f},
    };
    const DvzColor fan_colors[8] = {text, cyan, mint, warm, rose, cyan, mint, warm};
    return _add_primitive(
        scene, panel, DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN, fan_positions, fan_colors, 8);
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Run the retained primitive visual example.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @return process exit code
 */
int main(int argc, char** argv)
{
    const uint32_t frame_count = example_frame_count_any(argc, argv);
    DvzAppCaptureConfig capture = dvz_app_capture_config_from_env("visual_primitive");

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

    EXAMPLE_CHECK(_add_primitives(scene, panel), "primitive visual setup failed");

    app = dvz_app(scene);
    EXAMPLE_CHECK(app != NULL, "dvz_app() failed (no GPU or display?)");

    win = dvz_view_glfw(app, figure, WIDTH, HEIGHT, "visual_primitive");
    EXAMPLE_CHECK(win != NULL, "dvz_view_glfw() failed (GLFW unavailable?)");

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
