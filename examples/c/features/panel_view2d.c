/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* panel_view2d - panel-owned 2D view framing with equal aspect.
 *
 * Scenario: feature.panel_view2d
 * Style: features, graphite_cyan, 1600x1200 capture target
 *
 * Build:  just example-c features/panel_view2d
 * Run:    ./build/examples/c/features/panel_view2d --live
 * Smoke:  ./build/examples/c/features/panel_view2d --png
 */



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>
#include <math.h>

#include "_assertions.h"
#include "datoviz/scene.h"
#include "example_common.h"
#include "example_style.h"
#include "runner/scenario_runner.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH       1600u
#define HEIGHT      1200u
#define CIRCLE_COUNT 97u



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Add a unit circle in data coordinates.
 *
 * @param scene scene owning visuals
 * @param panel target panel
 * @param color rectangle color
 * @return true when visuals were added
 */
static bool _add_domain_shape(DvzScene* scene, DvzPanel* panel, DvzColor color)
{
    ANN(scene);
    ANN(panel);

    vec3 circle[CIRCLE_COUNT] = {0};
    DvzColor circle_colors[CIRCLE_COUNT] = {{0}};
    float circle_widths[CIRCLE_COUNT] = {0};
    for (uint32_t i = 0; i < CIRCLE_COUNT; i++)
    {
        const float t = (float)i / (float)(CIRCLE_COUNT - 1u);
        const float a = 6.283185307179586f * t;
        circle[i][0] = cosf(a);
        circle[i][1] = sinf(a);
        circle[i][2] = 0.0f;
        circle_colors[i] = color;
        circle_widths[i] = 4.0f;
    }

    DvzVisual* path = dvz_path(scene, 0);
    if (path == NULL)
        return false;
    DvzVisualDataUpdate path_updates[] = {
        {.attr_name = "position", .data = circle, .item_count = CIRCLE_COUNT},
        {.attr_name = "color", .data = circle_colors, .item_count = CIRCLE_COUNT},
        {.attr_name = "stroke_width_px", .data = circle_widths, .item_count = CIRCLE_COUNT},
    };
    if (dvz_visual_set_data_many(path, path_updates, 3) != 0)
        return false;
    if (dvz_path_set_caps(path, DVZ_SEGMENT_CAP_ROUND, DVZ_SEGMENT_CAP_ROUND) != 0)
        return false;
    if (dvz_path_set_join(path, DVZ_PATH_JOIN_ROUND, 4.0f) != 0)
        return false;
    DvzVisualAttachDesc path_attach = {
        DVZ_STRUCT_INIT_FIELDS(DvzVisualAttachDesc), .coord_space = DVZ_COORD_DATA};
    if (dvz_panel_add_visual(panel, path, &path_attach) != 0)
        return false;

    return true;
}



/**
 * Configure axes for the panel-view fit example.
 *
 * @param panel target panel
 * @return true when axes were configured
 */
static bool _add_axes(DvzPanel* panel)
{
    ANN(panel);

    DvzAxis* x_axis = dvz_panel_axis(panel, DVZ_DIM_X);
    DvzAxis* y_axis = dvz_panel_axis(panel, DVZ_DIM_Y);
    if (x_axis == NULL || y_axis == NULL)
        return false;
    if (!example_graphite_cyan_apply_axis_style(x_axis, false, NULL))
        return false;
    if (!example_graphite_cyan_apply_axis_style(y_axis, true, NULL))
        return false;
    DvzAxisTickPolicy ticks = dvz_axis_tick_policy();
    ticks.target_count = 5;
    ticks.min_pixel_spacing = 130.0f;
    ticks.minor_per_interval = 0;
    if (!dvz_axis_set_tick_policy(x_axis, &ticks) || !dvz_axis_set_tick_policy(y_axis, &ticks))
        return false;
    if (!dvz_axis_set_grid(x_axis, true) || !dvz_axis_set_grid(y_axis, true))
        return false;
    return dvz_axis_set_label(x_axis, "domain x") && dvz_axis_set_label(y_axis, "domain y");
}



/*************************************************************************************************/
/*  Scenario callbacks                                                                           */
/*************************************************************************************************/

/**
 * Initialize the panel-view fit feature example.
 *
 * @param ctx scenario context
 * @param out_user scenario state output
 * @return true on success
 */
static bool _scenario_init(DvzScenarioContext* ctx, void** out_user)
{
    if (ctx == NULL)
        return false;
    if (out_user != NULL)
        *out_user = NULL;

    ctx->figure = dvz_figure(ctx->scene, ctx->width, ctx->height, 0);
    if (ctx->figure == NULL)
        return false;

    DvzGrid* grid = dvz_figure_grid(ctx->figure, 1, 2);
    if (grid == NULL)
        return false;
    if (!example_configure_compact_grid(grid, 34.0f, 0.0f))
        return false;

    DvzPanel* free_panel = dvz_grid_panel(grid, 0, 0);
    DvzPanel* fit_panel = dvz_grid_panel(grid, 0, 1);
    if (free_panel == NULL || fit_panel == NULL)
        return false;

    DvzPanel* panels[2] = {free_panel, fit_panel};
    for (uint32_t i = 0; i < 2u; i++)
    {
        example_graphite_cyan_set_panel_background(panels[i]);
        if (!_add_axes(panels[i]))
            return false;
    }

    if (dvz_panel_set_domain(free_panel, DVZ_DIM_X, -1.0, +1.0) != 0)
        return false;
    if (dvz_panel_set_domain(free_panel, DVZ_DIM_Y, -1.0, +1.0) != 0)
        return false;

    if (!example_configure_equal_aspect_panel(
            fit_panel, (DvzDataDomain){.min = -1.0, .max = +1.0},
            (DvzDataDomain){.min = -1.0, .max = +1.0}, 0.18))
        return false;

    double x_min = 0.0;
    double x_max = 0.0;
    double y_min = 0.0;
    double y_max = 0.0;
    if (!dvz_panel_visible_domain(fit_panel, DVZ_DIM_X, &x_min, &x_max))
        return false;
    if (!dvz_panel_visible_domain(fit_panel, DVZ_DIM_Y, &y_min, &y_max))
        return false;
    const double x_span = x_max - x_min;
    const double y_span = y_max - y_min;
    DvzRect plot = {0};
    if (!dvz_panel_plot_rect_px(fit_panel, &plot) || !(plot.width > 0.0f) ||
        !(plot.height > 0.0f))
        return false;
    const double x_units_per_px = x_span / (double)plot.width;
    const double y_units_per_px = y_span / (double)plot.height;
    if (fabs(x_units_per_px - y_units_per_px) > 1e-4)
        return false;

    if (!_add_domain_shape(
            ctx->scene, free_panel,
            example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ACCENT_SECONDARY)))
        return false;
    if (!_add_domain_shape(
            ctx->scene, fit_panel,
            example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ACCENT_PRIMARY)))
        return false;
    DvzPanzoomDesc fit_panzoom = dvz_panzoom_desc();
    fit_panzoom.controller_flags = DVZ_PANZOOM_FLAGS_KEEP_ASPECT;
    return dvz_scenario_panzoom(ctx, free_panel, NULL, DVZ_DIM_MASK_XY) != NULL &&
           dvz_scenario_panzoom(ctx, fit_panel, &fit_panzoom, DVZ_DIM_MASK_XY) != NULL;
}



/**
 * Return the panel 2D view scenario specification.
 *
 * @return scenario specification
 */
static DvzScenarioSpec _panel_view2d_scenario(void)
{
    return (DvzScenarioSpec){
        .id = "feature_panel_view2d",
        .title = "panel_view2d",
        .width = WIDTH,
        .height = HEIGHT,
        .fps = 60.0,
        .init = _scenario_init,
    };
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Run the panel-view fit feature example through the native scenario runner.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @return process exit code
 */
int main(int argc, char** argv)
{
    DvzScenarioSpec spec = _panel_view2d_scenario();
    return dvz_scenario_run_native_cli(&spec, argc, argv) == 0 ? 0 : 1;
}
