/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* panel_linked - two panels with one-way linked X panzoom state.
 *
 * Scenario: feature.panel_linked
 * Style: features, graphite_cyan, 1600x1200 capture target
 *
 * Build:  just example-c features/panel_linked
 * Run:    ./build/examples/c/features/panel_linked
 * Smoke:  ./build/examples/c/features/panel_linked 1
 * PNG:    DVZ_CAPTURE=png ./build/examples/c/features/panel_linked 1
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

#define WIDTH      1600u
#define HEIGHT     1200u
#define PATH_COUNT 128u

static const float TAU = 6.28318530718f;



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Set the same X data domain and a panel-specific Y domain.
 *
 * @param panel target panel
 * @param ymin minimum Y domain value
 * @param ymax maximum Y domain value
 * @return true when both dimensions were set
 */
static bool _set_panel_domain(DvzPanel* panel, double ymin, double ymax)
{
    ANN(panel);

    int rc = dvz_panel_set_domain(panel, DVZ_DIM_X, 0.0, 10.0);
    if (rc != 0)
        return false;
    rc = dvz_panel_set_domain(panel, DVZ_DIM_Y, ymin, ymax);
    return rc == 0;
}



/**
 * Add one deterministic line panel.
 *
 * @param scene scene owning the visual
 * @param panel panel receiving the visual
 * @param phase signal phase offset
 * @param color_base base green channel for the line colors
 * @return true when the visual was added
 */
static bool _add_line_panel(DvzScene* scene, DvzPanel* panel, float phase, uint8_t color_base)
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
        data_positions[i][0] = 10.0f * t;
        data_positions[i][1] =
            0.62f * sinf(TAU * (1.05f * t + phase)) +
            0.22f * cosf(TAU * (2.30f * t + 0.17f + phase));
        data_positions[i][2] = 0.0f;
        colors[i] = dvz_color_rgba(70, (uint8_t)(color_base + 32.0f * t), 232, 242);
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
 * Run the linked-panel feature example.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @return process exit code
 */
int main(int argc, char** argv)
{
    const uint32_t frame_count = example_frame_count_any(argc, argv);
    DvzAppCaptureConfig capture = dvz_app_capture_config_from_env("feature_panel_linked");

    int ret = 1;
    DvzScene* scene = NULL;
    DvzApp* app = NULL;
    DvzView* win = NULL;

    scene = dvz_scene();
    EXAMPLE_CHECK(scene != NULL, "dvz_scene() failed");

    DvzFigure* figure = dvz_figure(scene, WIDTH, HEIGHT, 0);
    EXAMPLE_CHECK(figure != NULL, "dvz_figure() failed");

    DvzGrid* grid = dvz_figure_grid(figure, 2, 1);
    EXAMPLE_CHECK(grid != NULL, "dvz_figure_grid() failed");
    EXAMPLE_CHECK(
        dvz_grid_set_margins(
            grid, &(DvzPanelReserve){
                      .left_px = 100.0f, .right_px = 100.0f, .top_px = 90.0f,
                      .bottom_px = 90.0f}),
        "dvz_grid_set_margins() failed");
    EXAMPLE_CHECK(dvz_grid_set_gutter(grid, 0.0f, 40.0f), "dvz_grid_set_gutter() failed");

    DvzPanel* top = dvz_grid_panel(grid, 0, 0);
    DvzPanel* bottom = dvz_grid_panel(grid, 1, 0);
    EXAMPLE_CHECK(top != NULL && bottom != NULL, "dvz_grid_panel() failed");

    DvzPanel* panels[2] = {top, bottom};
    for (uint32_t i = 0; i < 2u; i++)
    {
        example_graphite_cyan_set_panel_background(panels[i]);
        DvzPanelBorderDesc border = dvz_panel_border_desc();
        border.color = example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_GRID);
        border.width_px = 1.5f;
        EXAMPLE_CHECK(dvz_panel_set_border(panels[i], &border), "dvz_panel_set_border() failed");
    }

    EXAMPLE_CHECK(_set_panel_domain(top, -1.1, 1.1), "top panel domain setup failed");
    EXAMPLE_CHECK(_set_panel_domain(bottom, -1.8, 1.8), "bottom panel domain setup failed");
    EXAMPLE_CHECK(_add_line_panel(scene, top, 0.03f, 188u), "top panel visual setup failed");
    EXAMPLE_CHECK(_add_line_panel(scene, bottom, 0.24f, 164u), "bottom panel visual setup failed");

    DvzController* source_x = dvz_panzoom(scene, NULL);
    DvzController* target_x = dvz_panzoom(scene, NULL);
    DvzController* top_y = dvz_panzoom(scene, NULL);
    DvzController* bottom_y = dvz_panzoom(scene, NULL);
    EXAMPLE_CHECK(
        source_x != NULL && target_x != NULL && top_y != NULL && bottom_y != NULL,
        "dvz_panzoom() failed");

    DvzPanzoom* source_panzoom = dvz_controller_panzoom(source_x);
    DvzPanzoom* target_panzoom = dvz_controller_panzoom(target_x);
    DvzPanzoom* top_y_panzoom = dvz_controller_panzoom(top_y);
    DvzPanzoom* bottom_y_panzoom = dvz_controller_panzoom(bottom_y);
    EXAMPLE_CHECK(
        source_panzoom != NULL && target_panzoom != NULL && top_y_panzoom != NULL &&
            bottom_y_panzoom != NULL,
        "dvz_controller_panzoom() failed");

    dvz_panzoom_zoom(source_panzoom, (vec2){1.80f, 1.0f});
    dvz_panzoom_pan(source_panzoom, (vec2){+0.22f, 0.0f});
    dvz_panzoom_zoom(top_y_panzoom, (vec2){1.0f, 1.15f});
    dvz_panzoom_zoom(bottom_y_panzoom, (vec2){1.0f, 1.45f});

    DvzControllerLink* link = dvz_controller_link(
        scene, source_x, target_x, DVZ_CONTROLLER_LINK_EXTENT_X, DVZ_CONTROLLER_LINK_ONE_WAY);
    EXAMPLE_CHECK(link != NULL, "dvz_controller_link() failed");
    EXAMPLE_CHECK(
        fabsf(target_panzoom->zoom[0] - source_panzoom->zoom[0]) < 1e-6f,
        "linked X zoom mismatch");

    app = dvz_app(scene);
    EXAMPLE_CHECK(app != NULL, "dvz_app() failed (no GPU or display?)");

    win = dvz_view_glfw(app, figure, WIDTH, HEIGHT, "panel_linked");
    EXAMPLE_CHECK(win != NULL, "dvz_view_glfw() failed (GLFW unavailable?)");

    EXAMPLE_CHECK(
        dvz_view_bind_controller(win, top, source_x, DVZ_DIM_MASK_X) == 0,
        "top X controller binding failed");
    EXAMPLE_CHECK(
        dvz_view_bind_controller(win, top, top_y, DVZ_DIM_MASK_Y) == 0,
        "top Y controller binding failed");
    EXAMPLE_CHECK(
        dvz_view_bind_controller(win, bottom, target_x, DVZ_DIM_MASK_X) == 0,
        "bottom X controller binding failed");
    EXAMPLE_CHECK(
        dvz_view_bind_controller(win, bottom, bottom_y, DVZ_DIM_MASK_Y) == 0,
        "bottom Y controller binding failed");

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
