/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* panel_multi - multiple independent panels with panel-local panzoom controllers.
 *
 * Scenario: feature.panel_multi
 * Style: features, graphite_cyan, 1600x1200 capture target
 *
 * Build:  just example-c features/panel_multi
 * Run:    ./build/examples/c/features/panel_multi
 * Smoke:  ./build/examples/c/features/panel_multi 1
 * PNG:    DVZ_CAPTURE=png ./build/examples/c/features/panel_multi 1
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
#define POINT_COUNT 48u
#define PATH_COUNT  96u

static const float TAU = 6.28318530718f;



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Set a compact 2D data domain on one panel.
 *
 * @param panel target panel
 * @return true when both dimensions were set
 */
static bool _set_unit_domain(DvzPanel* panel)
{
    ANN(panel);

    int rc = dvz_panel_set_domain(panel, DVZ_DIM_X, -1.0, 1.0);
    if (rc != 0)
        return false;
    rc = dvz_panel_set_domain(panel, DVZ_DIM_Y, -1.0, 1.0);
    return rc == 0;
}



/**
 * Add deterministic point data to one panel.
 *
 * @param scene scene owning the visual
 * @param panel panel receiving the visual
 * @return true when the visual was added
 */
static bool _add_point_panel(DvzScene* scene, DvzPanel* panel)
{
    ANN(scene);
    ANN(panel);

    vec3 data_positions[POINT_COUNT] = {{0}};
    vec3 visual_positions[POINT_COUNT] = {{0}};
    DvzColor colors[POINT_COUNT] = {{0}};
    float diameters[POINT_COUNT] = {0};

    DvzColor primary = example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ACCENT_PRIMARY);
    DvzColor secondary = example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ACCENT_SECONDARY);
    for (uint32_t i = 0; i < POINT_COUNT; i++)
    {
        const float t = (float)i / (float)POINT_COUNT;
        const float theta = TAU * t;
        const float radius = 0.34f + 0.18f * sinf(3.0f * theta);

        data_positions[i][0] = radius * cosf(theta);
        data_positions[i][1] = radius * sinf(theta);
        data_positions[i][2] = 0.0f;
        colors[i] = i % 2u == 0u ? primary : secondary;
        colors[i].a = 238u;
        diameters[i] = 7.0f + 7.0f * (0.5f + 0.5f * sinf(5.0f * theta));
    }

    int rc = dvz_panel_data_to_visual_positions(
        panel, (const float*)data_positions, (float*)visual_positions, POINT_COUNT);
    if (rc != 0)
        return false;

    DvzVisual* visual = dvz_point(scene, 0);
    if (visual == NULL)
        return false;
    DvzVisualDataUpdate updates[] = {
        {.attr_name = "position", .data = visual_positions, .item_count = POINT_COUNT},
        {.attr_name = "color", .data = colors, .item_count = POINT_COUNT},
        {.attr_name = "diameter", .data = diameters, .item_count = POINT_COUNT},
    };
    if (dvz_visual_set_data_many(visual, updates, 3) != 0)
        return false;

    DvzPointStyleDesc style = dvz_point_style_desc();
    style.aspect = DVZ_SHAPE_ASPECT_FILLED;
    style.stroke_width = 0.0f;
    if (dvz_point_set_style(visual, &style) != 0)
        return false;
    if (dvz_visual_set_depth_test(visual, false) != 0)
        return false;
    return dvz_panel_add_visual(panel, visual, NULL) == 0;
}



/**
 * Add deterministic path data to one panel.
 *
 * @param scene scene owning the visual
 * @param panel panel receiving the visual
 * @return true when the visual was added
 */
static bool _add_path_panel(DvzScene* scene, DvzPanel* panel)
{
    ANN(scene);
    ANN(panel);

    vec3 data_positions[PATH_COUNT] = {{0}};
    vec3 visual_positions[PATH_COUNT] = {{0}};
    DvzColor colors[PATH_COUNT] = {{0}};
    float widths[PATH_COUNT] = {0};

    for (uint32_t i = 0; i < PATH_COUNT; i++)
    {
        const float t = PATH_COUNT > 1u ? (float)i / (float)(PATH_COUNT - 1u) : 0.0f;
        data_positions[i][0] = -0.88f + 1.76f * t;
        data_positions[i][1] = 0.32f * sinf(TAU * (1.5f * t + 0.08f)) +
                               0.18f * cosf(TAU * (3.0f * t + 0.21f));
        data_positions[i][2] = 0.0f;
        colors[i] = dvz_color_rgba(74, (uint8_t)(176.0f + 56.0f * t), 232, 242);
        widths[i] = 3.0f;
    }

    int rc = dvz_panel_data_to_visual_positions(
        panel, (const float*)data_positions, (float*)visual_positions, PATH_COUNT);
    if (rc != 0)
        return false;

    DvzVisual* visual = dvz_path(scene, 0);
    if (visual == NULL)
        return false;
    DvzVisualDataUpdate updates[] = {
        {.attr_name = "position", .data = visual_positions, .item_count = PATH_COUNT},
        {.attr_name = "color", .data = colors, .item_count = PATH_COUNT},
        {.attr_name = "stroke_width", .data = widths, .item_count = PATH_COUNT},
    };
    if (dvz_visual_set_data_many(visual, updates, 3) != 0)
        return false;
    if (dvz_path_set_caps(visual, DVZ_SEGMENT_CAP_ROUND, DVZ_SEGMENT_CAP_ROUND) != 0)
        return false;
    if (dvz_path_set_join(visual, DVZ_PATH_JOIN_ROUND, 4.0f) != 0)
        return false;
    if (dvz_visual_set_depth_test(visual, false) != 0)
        return false;
    return dvz_panel_add_visual(panel, visual, NULL) == 0;
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Run the multiple-independent-panel feature example.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @return process exit code
 */
int main(int argc, char** argv)
{
    const uint32_t frame_count = example_frame_count_any(argc, argv);
    DvzAppCaptureConfig capture = dvz_app_capture_config_from_env("feature_panel_multi");

    int ret = 1;
    DvzScene* scene = NULL;
    DvzApp* app = NULL;
    DvzView* win = NULL;

    scene = dvz_scene();
    EXAMPLE_CHECK(scene != NULL, "dvz_scene() failed");

    DvzFigure* figure = dvz_figure(scene, WIDTH, HEIGHT, 0);
    EXAMPLE_CHECK(figure != NULL, "dvz_figure() failed");

    DvzPanel* left =
        dvz_panel(figure, (DvzPanelDesc){.x = 0.08f, .y = 0.16f, .width = 0.40f, .height = 0.68f});
    DvzPanel* right =
        dvz_panel(figure, (DvzPanelDesc){.x = 0.52f, .y = 0.16f, .width = 0.40f, .height = 0.68f});
    EXAMPLE_CHECK(left != NULL && right != NULL, "dvz_panel() failed");

    DvzPanel* panels[2] = {left, right};
    for (uint32_t i = 0; i < 2u; i++)
    {
        example_graphite_cyan_set_panel_background(panels[i]);
        DvzPanelBorderDesc border = dvz_panel_border_desc();
        border.color = example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_GRID);
        border.width_px = 1.5f;
        EXAMPLE_CHECK(dvz_panel_set_border(panels[i], &border), "dvz_panel_set_border() failed");
        EXAMPLE_CHECK(_set_unit_domain(panels[i]), "panel domain setup failed");
    }

    EXAMPLE_CHECK(_add_point_panel(scene, left), "left panel visual setup failed");
    EXAMPLE_CHECK(_add_path_panel(scene, right), "right panel visual setup failed");

    app = dvz_app(scene);
    EXAMPLE_CHECK(app != NULL, "dvz_app() failed (no GPU or display?)");

    win = dvz_view_glfw(app, figure, WIDTH, HEIGHT, "panel_multi");
    EXAMPLE_CHECK(win != NULL, "dvz_view_glfw() failed (GLFW unavailable?)");

    DvzPanzoom* left_panzoom = dvz_view_panzoom(win, left, NULL);
    DvzPanzoom* right_panzoom = dvz_view_panzoom(win, right, NULL);
    EXAMPLE_CHECK(
        left_panzoom != NULL && right_panzoom != NULL, "failed to create panel panzooms");
    dvz_panzoom_zoom(left_panzoom, (vec2){1.20f, 1.20f});
    dvz_panzoom_pan(left_panzoom, (vec2){-0.12f, +0.08f});
    dvz_panzoom_zoom(right_panzoom, (vec2){1.65f, 1.10f});
    dvz_panzoom_pan(right_panzoom, (vec2){+0.18f, -0.04f});

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
