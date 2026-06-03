/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* alpha_blending - retained primitive visual using per-vertex alpha and source-over blending.
 *
 * Scenario: feature.alpha_blending
 * Style: features, graphite_cyan, 1600x1200 capture target
 *
 * The primitive visual uses RGBA vertex colors and DVZ_ALPHA_BLENDED so triangle overlaps mix with
 * the graphite-cyan panel background and with each other.
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

#define WIDTH        1600u
#define HEIGHT       1200u
#define VERTEX_COUNT 9u



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Fill overlapping translucent triangle data.
 *
 * @param positions output vertex positions
 * @param normals output vertex normals
 * @param colors output vertex colors
 */
static void _fill_triangles(
    vec3 positions[VERTEX_COUNT], vec3 normals[VERTEX_COUNT], DvzColor colors[VERTEX_COUNT])
{
    const vec3 triangle_positions[VERTEX_COUNT] = {
        {-0.76f, -0.54f, 0.00f}, {-0.06f, +0.76f, 0.00f}, {+0.34f, -0.42f, 0.00f},
        {-0.34f, +0.48f, 0.01f}, {+0.82f, +0.36f, 0.01f}, {+0.08f, -0.82f, 0.01f},
        {-0.86f, +0.10f, 0.02f}, {+0.24f, +0.90f, 0.02f}, {+0.78f, -0.10f, 0.02f},
    };
    const ExampleStyleColorRole roles[3] = {
        EXAMPLE_STYLE_COLOR_ACCENT_PRIMARY,
        EXAMPLE_STYLE_COLOR_ACCENT_SECONDARY,
        EXAMPLE_STYLE_COLOR_WARNING,
    };
    const uint8_t alpha[3] = {150, 132, 118};

    for (uint32_t i = 0; i < VERTEX_COUNT; i++)
    {
        positions[i][0] = triangle_positions[i][0];
        positions[i][1] = triangle_positions[i][1];
        positions[i][2] = triangle_positions[i][2];
        normals[i][0] = 0.0f;
        normals[i][1] = 0.0f;
        normals[i][2] = 1.0f;

        const uint32_t triangle = i / 3u;
        colors[i] = example_graphite_cyan_color(roles[triangle]);
        colors[i].a = alpha[triangle];
    }
}



/**
 * Add one source-over blended primitive visual to the panel.
 *
 * @param scene scene owning the visual
 * @param panel panel receiving the visual
 * @return true when the visual was added
 */
static bool _add_blended_triangles(DvzScene* scene, DvzPanel* panel)
{
    vec3 positions[VERTEX_COUNT] = {{0}};
    vec3 normals[VERTEX_COUNT] = {{0}};
    DvzColor colors[VERTEX_COUNT] = {{0}};
    _fill_triangles(positions, normals, colors);

    DvzVisual* visual = dvz_primitive(scene, DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0);
    if (visual == NULL)
        return false;

    DvzVisualDataUpdate updates[] = {
        {.attr_name = "position", .data = positions, .item_count = VERTEX_COUNT},
        {.attr_name = "normal", .data = normals, .item_count = VERTEX_COUNT},
        {.attr_name = "color", .data = colors, .item_count = VERTEX_COUNT},
    };
    if (dvz_visual_set_data_many(visual, updates, 3) != 0)
        return false;
    if (dvz_visual_set_alpha_mode(visual, DVZ_ALPHA_BLENDED) != 0)
        return false;

    return dvz_panel_add_visual(panel, visual, NULL) == 0;
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Run the source-over alpha-blending feature example.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @return process exit code
 */
int main(int argc, char** argv)
{
    const uint32_t frame_count = example_frame_count_any(argc, argv);
    DvzAppCaptureConfig capture = dvz_app_capture_config_from_env("feature_alpha_blending");

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

    EXAMPLE_CHECK(_add_blended_triangles(scene, panel), "blended triangle setup failed");

    app = dvz_app(scene);
    EXAMPLE_CHECK(app != NULL, "dvz_app() failed (no GPU or display?)");

    win = dvz_view_glfw(app, figure, WIDTH, HEIGHT, "feature_alpha_blending");
    EXAMPLE_CHECK(win != NULL, "dvz_view_glfw() failed (GLFW unavailable?)");

    EXAMPLE_CHECK(dvz_view_capture_start(win, &capture) == 0, "dvz_view_capture_start() failed");
    capture_started = true;

    dvz_app_run(app, frame_count);

    EXAMPLE_CHECK(dvz_view_capture_stop(win) == 0, "dvz_view_capture_stop() failed");
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
