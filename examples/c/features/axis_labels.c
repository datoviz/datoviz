/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* axis_labels - retained axis titles and tick-label placement with plot margins.
 *
 * Scenario: feature.axis_labels
 * Style: features, graphite_cyan, 1600x1200 capture target
 *
 * Build:  just example-c features/axis_labels
 * Run:    ./build/examples/c/features/axis_labels
 * Smoke:  ./build/examples/c/features/axis_labels 1
 * PNG:    DVZ_CAPTURE=png ./build/examples/c/features/axis_labels 1
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

#define WIDTH         1600u
#define HEIGHT        1200u
#define SEGMENT_COUNT 10u



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Add deterministic plot-area guides that align with the axis plot margins.
 *
 * @param scene scene owning the guide visual
 * @param panel panel receiving the guide visual
 * @return true when the guides were added
 */
static bool _add_plot_guides(DvzScene* scene, DvzPanel* panel)
{
    ANN(scene);
    ANN(panel);

    const float left = -0.70f;
    const float right = +0.78f;
    const float bottom = -0.66f;
    const float top = +0.74f;

    vec3 starts[SEGMENT_COUNT] = {
        {left, bottom, 0.0f}, {right, bottom, 0.0f}, {right, top, 0.0f}, {left, top, 0.0f},
        {left, 0.0f, 0.0f},  {0.0f, bottom, 0.0f}, {left, -0.34f, 0.0f},
        {left, +0.34f, 0.0f}, {+0.24f, bottom, 0.0f}, {-0.24f, bottom, 0.0f},
    };
    vec3 ends[SEGMENT_COUNT] = {
        {right, bottom, 0.0f}, {right, top, 0.0f}, {left, top, 0.0f}, {left, bottom, 0.0f},
        {right, 0.0f, 0.0f},  {0.0f, top, 0.0f}, {right, +0.48f, 0.0f},
        {right, -0.18f, 0.0f}, {+0.58f, top, 0.0f}, {-0.58f, top, 0.0f},
    };
    DvzColor colors[SEGMENT_COUNT] = {0};
    float widths[SEGMENT_COUNT] = {0};

    for (uint32_t i = 0; i < SEGMENT_COUNT; i++)
    {
        colors[i] = i < 4u ? example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ACCENT_PRIMARY)
                            : example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ACCENT_SECONDARY);
        colors[i].a = i < 4u ? 220u : 180u;
        widths[i] = i < 4u ? 3.0f : 2.0f;
    }

    DvzVisual* visual = dvz_segment(scene, 0);
    if (visual == NULL)
        return false;

    DvzVisualDataUpdate updates[] = {
        {.attr_name = "position_start", .data = starts, .item_count = SEGMENT_COUNT},
        {.attr_name = "position_end", .data = ends, .item_count = SEGMENT_COUNT},
        {.attr_name = "color", .data = colors, .item_count = SEGMENT_COUNT},
        {.attr_name = "stroke_width", .data = widths, .item_count = SEGMENT_COUNT},
    };
    if (dvz_visual_set_data_many(visual, updates, 4) != 0)
        return false;
    if (dvz_segment_set_caps(visual, DVZ_SEGMENT_CAP_SQUARE, DVZ_SEGMENT_CAP_SQUARE) != 0)
        return false;
    return dvz_panel_add_visual(panel, visual, NULL) == 0;
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Run the axis label placement feature example.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @return process exit code
 */
int main(int argc, char** argv)
{
    const uint32_t frame_count = example_frame_count_any(argc, argv);
    DvzAppCaptureConfig capture = dvz_app_capture_config_from_env("axis_labels");

    int ret = 1;
    DvzScene* scene = NULL;
    DvzApp* app = NULL;

    scene = dvz_scene();
    EXAMPLE_CHECK(scene != NULL, "dvz_scene() failed");

    DvzFigure* figure = dvz_figure(scene, WIDTH, HEIGHT, 0);
    EXAMPLE_CHECK(figure != NULL, "dvz_figure() failed");

    DvzPanel* panel = dvz_panel_full(figure);
    EXAMPLE_CHECK(panel != NULL, "dvz_panel_full() failed");
    example_graphite_cyan_set_panel_background(panel);

    bool ok = dvz_panel_set_layout_reserve(
        panel, &(DvzPanelLayoutReserve){.left = 0.18f, .right = 0.08f, .bottom = 0.19f,
                                        .top = 0.08f});
    EXAMPLE_CHECK(ok, "dvz_panel_set_layout_reserve() failed");

    EXAMPLE_CHECK(dvz_panel_set_domain(panel, DVZ_DIM_X, -40.0, 120.0) == 0, "set X domain failed");
    EXAMPLE_CHECK(dvz_panel_set_domain(panel, DVZ_DIM_Y, -1.5, 2.5) == 0, "set Y domain failed");
    EXAMPLE_CHECK(_add_plot_guides(scene, panel), "plot guide setup failed");

    DvzAxis* x_axis = dvz_panel_axis(panel, DVZ_DIM_X);
    DvzAxis* y_axis = dvz_panel_axis(panel, DVZ_DIM_Y);
    EXAMPLE_CHECK(x_axis != NULL && y_axis != NULL, "dvz_panel_axis() failed");

    DvzAxisTickPolicy ticks = dvz_axis_tick_policy();
    ticks.target_count = 5;
    ticks.min_pixel_spacing = 150.0f;
    ticks.minor_per_interval = 2;
    EXAMPLE_CHECK(dvz_axis_set_tick_policy(x_axis, &ticks), "X tick policy failed");
    EXAMPLE_CHECK(dvz_axis_set_tick_policy(y_axis, &ticks), "Y tick policy failed");

    ExampleAxisStyleOptions style = example_graphite_cyan_axis_options();
    style.tick_size_px = 14.0f;
    style.label_size_px = 20.0f;
    style.tick_gap_px = 10.0f;
    style.x_label_gap_px = 48.0f;
    style.y_label_gap_px = 70.0f;
    style.grid_alpha = 130u;
    EXAMPLE_CHECK(
        example_graphite_cyan_apply_axis_style(x_axis, false, &style), "X axis style failed");
    EXAMPLE_CHECK(
        example_graphite_cyan_apply_axis_style(y_axis, true, &style), "Y axis style failed");

    EXAMPLE_CHECK(dvz_axis_set_plot_margins(x_axis, 0.30f, 0.22f, 0.34f, 0.26f), "X margins failed");
    EXAMPLE_CHECK(dvz_axis_set_plot_margins(y_axis, 0.30f, 0.22f, 0.34f, 0.26f), "Y margins failed");
    EXAMPLE_CHECK(dvz_axis_set_grid(x_axis, true), "X grid failed");
    EXAMPLE_CHECK(dvz_axis_set_grid(y_axis, true), "Y grid failed");
    EXAMPLE_CHECK(dvz_axis_set_label(x_axis, "sample offset (ms)"), "X axis label failed");
    EXAMPLE_CHECK(dvz_axis_set_label(y_axis, "normalized response"), "Y axis label failed");

    app = dvz_app(scene);
    EXAMPLE_CHECK(app != NULL, "dvz_app() failed (no GPU or display?)");

    DvzView* win = dvz_view_glfw(app, figure, WIDTH, HEIGHT, "axis_labels");
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
