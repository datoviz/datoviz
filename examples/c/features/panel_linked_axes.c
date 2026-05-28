/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* panel_linked_axes - two stacked panels with shared X panzoom and retained axes.
 *
 * Scenario: linked_panels_axes_panzoom
 * Style: features, graphite_cyan, 1280x960 capture target
 *
 * Build:  just example-c features/panel_linked_axes
 * Run:    ./build/examples/c/features/panel_linked_axes
 * Smoke:  ./build/examples/c/features/panel_linked_axes 1
 */



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

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
#define PATH_COUNT  320u
#define POINT_COUNT 144u

static const float TAU = 6.28318530718f;



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Fill deterministic path data in data coordinates.
 *
 * @param positions output data-space path positions
 * @param colors output path colors
 * @param widths output path stroke widths
 * @param count sample count
 */
static void _fill_path(vec3* positions, DvzColor* colors, float* widths, uint32_t count)
{
    ANN(positions);
    ANN(colors);
    ANN(widths);

    const float inv_count = count > 1 ? 1.0f / (float)(count - 1u) : 1.0f;
    for (uint32_t i = 0; i < count; i++)
    {
        const float t = (float)i * inv_count;
        const float x = 12.0f * t;
        const float y = 0.92f * sinf(TAU * (1.15f * t + 0.06f)) +
                        0.28f * sinf(TAU * (3.50f * t + 0.21f));
        positions[i][0] = x;
        positions[i][1] = y;
        positions[i][2] = 0.0f;

        colors[i] = dvz_color_rgba(76, (uint8_t)(188.0f + 36.0f * t), 240, 255);
        widths[i] = 3.5f;
    }
}



/**
 * Fill deterministic point data in data coordinates.
 *
 * @param positions output data-space point positions
 * @param colors output point colors
 * @param diameters output point diameters
 * @param count point count
 */
static void _fill_points(vec3* positions, DvzColor* colors, float* diameters, uint32_t count)
{
    ANN(positions);
    ANN(colors);
    ANN(diameters);

    for (uint32_t i = 0; i < count; i++)
    {
        const uint32_t row = i / 24u;
        const uint32_t col = i % 24u;
        const float tx = (float)col / 23.0f;
        const float ty = (float)row / 5.0f;
        const float x = 12.0f * tx;
        const float ridge = sinf(TAU * (0.92f * tx + 0.13f * ty));
        const float y = -1.35f + 2.70f * ty + 0.20f * ridge;

        positions[i][0] = x;
        positions[i][1] = y;
        positions[i][2] = 0.0f;

        const uint8_t g = (uint8_t)(154.0f + 76.0f * ty);
        const uint8_t b = (uint8_t)(190.0f + 45.0f * (1.0f - tx));
        colors[i] = dvz_color_rgba(128, g, b, 235);
        diameters[i] = 5.0f + 3.0f * (0.5f + 0.5f * ridge);
    }
}



/**
 * Apply the shared graphite-cyan axis style.
 *
 * @param axis axis to style
 * @param vertical whether this is the vertical axis
 * @return true when the style update succeeds
 */
static bool _style_axis(DvzAxis* axis, bool vertical)
{
    ANN(axis);

    DvzAxisStyle style = dvz_axis_style();
    style.spine_width = 1.5f;
    style.major_tick_width = 1.5f;
    style.minor_tick_width = 1.0f;
    style.grid_width = 1.0f;
    style.tick_size_px = 12.0f;
    style.label_size_px = 15.0f;
    style.tick_gap_px = 7.0f;
    style.label_gap_px = vertical ? 56.0f : 34.0f;
    style.spine_color[0] = 201;
    style.spine_color[1] = 209;
    style.spine_color[2] = 217;
    style.spine_color[3] = 255;
    style.major_tick_color[0] = 201;
    style.major_tick_color[1] = 209;
    style.major_tick_color[2] = 217;
    style.major_tick_color[3] = 255;
    style.minor_tick_color[0] = 140;
    style.minor_tick_color[1] = 151;
    style.minor_tick_color[2] = 165;
    style.minor_tick_color[3] = 220;
    style.grid_color[0] = 48;
    style.grid_color[1] = 54;
    style.grid_color[2] = 61;
    style.grid_color[3] = 150;
    style.show_grid = true;
    return dvz_axis_set_style(axis, &style);
}



/**
 * Configure retained X/Y axes on one panel.
 *
 * @param panel target panel
 * @param x_label X axis label
 * @param y_label Y axis label
 * @return true when all axis calls succeed
 */
static bool _add_axes(DvzPanel* panel, const char* x_label, const char* y_label)
{
    ANN(panel);
    ANN(y_label);

    DvzAxis* x_axis = dvz_panel_axis(panel, DVZ_DIM_X);
    DvzAxis* y_axis = dvz_panel_axis(panel, DVZ_DIM_Y);
    if (x_axis == NULL || y_axis == NULL)
        return false;

    DvzAxisTickPolicy ticks = dvz_axis_tick_policy();
    ticks.target_count = 6;
    ticks.min_pixel_spacing = 100.0f;
    ticks.minor_per_interval = 3;
    if (!dvz_axis_set_tick_policy(x_axis, &ticks))
        return false;
    if (!dvz_axis_set_tick_policy(y_axis, &ticks))
        return false;
    if (!_style_axis(x_axis, false) || !_style_axis(y_axis, true))
        return false;
    if (!dvz_axis_set_grid(x_axis, true) || !dvz_axis_set_grid(y_axis, true))
        return false;
    if (x_label != NULL && !dvz_axis_set_label(x_axis, x_label))
        return false;
    return dvz_axis_set_label(y_axis, y_label);
}



/**
 * Add a stroked path visual to one panel.
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
    _fill_path(data_positions, colors, widths, PATH_COUNT);

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



/**
 * Add a point visual to one panel.
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
    _fill_points(data_positions, colors, diameters, POINT_COUNT);

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
    if (dvz_point_set_style(visual, &style) != 0)
        return false;
    if (dvz_visual_set_depth_test(visual, false) != 0)
        return false;
    return dvz_panel_add_visual(panel, visual, NULL) == 0;
}



/**
 * Bind one shared X panzoom and independent Y panzooms to both panels.
 *
 * @param scene scene owning the controllers
 * @param win view owning the input router
 * @param top top panel
 * @param bottom bottom panel
 * @return true when controllers and input routing are ready
 */
static bool _bind_linked_panzooms(DvzScene* scene, DvzView* win, DvzPanel* top, DvzPanel* bottom)
{
    ANN(scene);
    ANN(win);
    ANN(top);
    ANN(bottom);

    DvzController* shared_x = dvz_panzoom(scene, NULL);
    DvzController* top_y = dvz_panzoom(scene, NULL);
    DvzController* bottom_y = dvz_panzoom(scene, NULL);
    if (shared_x == NULL || top_y == NULL || bottom_y == NULL)
        return false;

    if (dvz_panel_bind_controller(top, shared_x, DVZ_DIM_MASK_X) != 0)
        return false;
    if (dvz_panel_bind_controller(bottom, shared_x, DVZ_DIM_MASK_X) != 0)
        return false;
    if (dvz_panel_bind_controller(top, top_y, DVZ_DIM_MASK_Y) != 0)
        return false;
    if (dvz_panel_bind_controller(bottom, bottom_y, DVZ_DIM_MASK_Y) != 0)
        return false;

    DvzInputRouter* router = dvz_view_input(win);
    if (router == NULL)
        return false;
    if (dvz_panel_connect_input(top, router) != 0)
        return false;
    return dvz_panel_connect_input(bottom, router) == 0;
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Run the linked-panel axes and shared-X panzoom feature proof.
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

    scene = dvz_scene();
    EXAMPLE_CHECK(scene != NULL, "dvz_scene() failed");

    DvzFigure* figure = dvz_figure(scene, WIDTH, HEIGHT, 0);
    EXAMPLE_CHECK(figure != NULL, "dvz_figure() failed");

    DvzGrid* grid = dvz_figure_grid(figure, 2, 1);
    EXAMPLE_CHECK(grid != NULL, "dvz_figure_grid() failed");
    bool ok = dvz_grid_set_margins(
        grid, &(DvzPanelReserve){.left_px = 28.0f, .right_px = 28.0f, .top_px = 28.0f,
                                 .bottom_px = 30.0f});
    EXAMPLE_CHECK(ok, "dvz_grid_set_margins() failed");
    ok = dvz_grid_set_gutter(grid, 0.0f, 28.0f);
    EXAMPLE_CHECK(ok, "dvz_grid_set_gutter() failed");

    DvzPanel* top = dvz_grid_panel(grid, 0, 0);
    DvzPanel* bottom = dvz_grid_panel(grid, 1, 0);
    EXAMPLE_CHECK(top != NULL && bottom != NULL, "dvz_grid_panel() failed");

    dvz_panel_set_background_color(top, 0.055f, 0.067f, 0.090f, 1.0f);
    dvz_panel_set_background_color(bottom, 0.055f, 0.067f, 0.090f, 1.0f);
    ok = dvz_panel_set_layout_reserve(
        top, &(DvzPanelLayoutReserve){.left = 0.15f, .right = 0.04f, .bottom = 0.13f,
                                      .top = 0.04f});
    EXAMPLE_CHECK(ok, "dvz_panel_set_layout_reserve(top) failed");
    ok = dvz_panel_set_layout_reserve(
        bottom, &(DvzPanelLayoutReserve){.left = 0.15f, .right = 0.04f, .bottom = 0.16f,
                                         .top = 0.04f});
    EXAMPLE_CHECK(ok, "dvz_panel_set_layout_reserve(bottom) failed");

    dvz_panel_set_domain(top, DVZ_DIM_X, 0.0, 12.0);
    dvz_panel_set_domain(bottom, DVZ_DIM_X, 0.0, 12.0);
    dvz_panel_set_domain(top, DVZ_DIM_Y, -1.6, 1.6);
    dvz_panel_set_domain(bottom, DVZ_DIM_Y, -2.0, 2.0);

    ok = _add_path_panel(scene, top);
    EXAMPLE_CHECK(ok, "_add_path_panel() failed");
    ok = _add_point_panel(scene, bottom);
    EXAMPLE_CHECK(ok, "_add_point_panel() failed");
    ok = _add_axes(top, NULL, "signal");
    EXAMPLE_CHECK(ok, "_add_axes(top) failed");
    ok = _add_axes(bottom, "time (s)", "samples");
    EXAMPLE_CHECK(ok, "_add_axes(bottom) failed");

    app = dvz_app(scene);
    EXAMPLE_CHECK(app != NULL, "dvz_app() failed (no GPU or display?)");

    DvzView* win = dvz_view_glfw(app, figure, WIDTH, HEIGHT, "linked_panels_axes_panzoom");
    EXAMPLE_CHECK(win != NULL, "dvz_view_glfw() failed (GLFW unavailable?)");

    ok = _bind_linked_panzooms(scene, win, top, bottom);
    EXAMPLE_CHECK(ok, "_bind_linked_panzooms() failed");

    dvz_app_run(app, example_frame_count(argc, argv));
    ret = 0;

cleanup:
    if (app != NULL)
        dvz_app_destroy(app);
    if (scene != NULL)
        dvz_scene_destroy(scene);
    return ret;
}
