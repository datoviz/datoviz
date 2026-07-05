/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* multi_window - one app driving two native GLFW windows.
 *
 * Scenario: feature.multi_window
 * Style: runtime, native app, multi-window
 *
 * Build:  just example-c runtime/multi_window
 * Run:    ./build/examples/c/runtime/multi_window
 * Smoke:  ./build/examples/c/runtime/multi_window 2
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

#define WIDTH  720u
#define HEIGHT 520u
#define POINT_COUNT 8u
#define FIRST_WINDOW_X  64
#define FIRST_WINDOW_Y  96
#define WINDOW_GAP_X    32



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Add one point cloud visual to a panel.
 *
 * @param scene scene owning the visual
 * @param panel destination panel
 * @param positions point positions
 * @param colors point colors
 * @param diameters point diameters in pixels
 * @return true on success
 */
static bool _add_points(
    DvzScene* scene, DvzPanel* panel, vec3* positions, DvzColor* colors, float* diameters)
{
    DvzVisual* point = dvz_point(scene, 0);
    if (point == NULL)
        return false;

    DvzVisualDataUpdate updates[] = {
        {.attr_name = "position", .data = positions, .item_count = POINT_COUNT},
        {.attr_name = "color", .data = colors, .item_count = POINT_COUNT},
        {.attr_name = "diameter_px", .data = diameters, .item_count = POINT_COUNT},
    };
    if (dvz_visual_set_data_many(point, updates, 3) != 0)
        return false;

    return dvz_panel_add_visual(panel, point, NULL) == 0;
}


/**
 * Create one positioned native GLFW view.
 *
 * @param app app driving the view
 * @param figure figure rendered in the view
 * @param title native window title
 * @param x initial monitor-space X position
 * @param y initial monitor-space Y position
 * @return view handle, or NULL
 */
static DvzView*
_positioned_view(DvzApp* app, DvzFigure* figure, const char* title, int32_t x, int32_t y)
{
    DvzViewDesc desc = dvz_view_desc(DVZ_VIEW_WINDOW);
    desc.logical_width = WIDTH;
    desc.logical_height = HEIGHT;
    desc.title = title;
    desc.has_position = true;
    desc.x = x;
    desc.y = y;
    return dvz_view(app, figure, &desc);
}



/*************************************************************************************************/
/*  Main                                                                                         */
/*************************************************************************************************/

int main(int argc, char** argv)
{
    int ret = 1;
    DvzScene* scene = NULL;
    DvzApp* app = NULL;

    scene = dvz_scene();
    EXAMPLE_CHECK(scene != NULL, "dvz_scene() failed");

    DvzFigure* overview = dvz_figure(scene, WIDTH, HEIGHT, 0);
    DvzFigure* detail = dvz_figure(scene, WIDTH, HEIGHT, 0);
    DvzPanel* overview_panel = overview != NULL ? dvz_panel_full(overview) : NULL;
    DvzPanel* detail_panel = detail != NULL ? dvz_panel_full(detail) : NULL;
    EXAMPLE_CHECK(
        overview != NULL && detail != NULL && overview_panel != NULL && detail_panel != NULL,
        "failed to create figures and panels");

    example_graphite_cyan_set_panel_background(overview_panel);
    example_graphite_cyan_set_panel_background(detail_panel);

    vec3 overview_positions[POINT_COUNT] = {
        {-0.78f, -0.40f, 0.0f}, {-0.55f, +0.20f, 0.0f}, {-0.32f, -0.08f, 0.0f},
        {-0.10f, +0.48f, 0.0f}, {+0.16f, -0.26f, 0.0f}, {+0.38f, +0.36f, 0.0f},
        {+0.62f, -0.02f, 0.0f}, {+0.82f, +0.30f, 0.0f},
    };
    DvzColor overview_colors[POINT_COUNT] = {
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ACCENT_PRIMARY),
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ACCENT_SECONDARY),
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_WARNING),
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_TEXT),
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_GRID),
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_MINOR_TICK),
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ACCENT_PRIMARY),
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ACCENT_SECONDARY),
    };
    float overview_diameters[POINT_COUNT] = {26.0f, 34.0f, 42.0f, 54.0f, 46.0f, 38.0f, 30.0f,
                                             24.0f};
    EXAMPLE_CHECK(
        _add_points(
            scene, overview_panel, overview_positions, overview_colors, overview_diameters),
        "failed to create overview points");

    vec3 detail_positions[POINT_COUNT] = {
        {-0.42f, -0.35f, 0.0f}, {-0.34f, +0.10f, 0.0f}, {-0.20f, +0.38f, 0.0f},
        {-0.02f, -0.08f, 0.0f}, {+0.14f, +0.52f, 0.0f}, {+0.30f, -0.28f, 0.0f},
        {+0.48f, +0.16f, 0.0f}, {+0.62f, +0.42f, 0.0f},
    };
    DvzColor detail_colors[POINT_COUNT] = {
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_WARNING),
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ACCENT_PRIMARY),
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_TEXT),
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ACCENT_SECONDARY),
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_GRID),
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_MINOR_TICK),
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_WARNING),
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ACCENT_PRIMARY),
    };
    float detail_diameters[POINT_COUNT] = {36.0f, 44.0f, 58.0f, 50.0f, 42.0f, 34.0f, 28.0f,
                                           24.0f};
    EXAMPLE_CHECK(
        _add_points(scene, detail_panel, detail_positions, detail_colors, detail_diameters),
        "failed to create detail points");

    app = dvz_app(scene);
    EXAMPLE_CHECK(app != NULL, "dvz_app() failed (no GPU or display?)");

    DvzView* overview_view =
        _positioned_view(app, overview, "multi_window overview", FIRST_WINDOW_X, FIRST_WINDOW_Y);
    DvzView* detail_view = _positioned_view(
        app, detail, "multi_window detail", FIRST_WINDOW_X + (int32_t)WIDTH + WINDOW_GAP_X,
        FIRST_WINDOW_Y);
    EXAMPLE_CHECK(
        overview_view != NULL && detail_view != NULL,
        "dvz_view_window() failed (GLFW unavailable?)");

    EXAMPLE_CHECK(
        dvz_view_panzoom(overview_view, overview_panel, NULL) != NULL,
        "overview dvz_view_panzoom() failed");
    EXAMPLE_CHECK(
        dvz_view_panzoom(detail_view, detail_panel, NULL) != NULL,
        "detail dvz_view_panzoom() failed");

    dvz_app_run(app, example_frame_count(argc, argv));
    ret = 0;

cleanup:
    if (app != NULL)
        dvz_app_destroy(app);
    if (scene != NULL)
        dvz_scene_destroy(scene);
    return ret;
}
