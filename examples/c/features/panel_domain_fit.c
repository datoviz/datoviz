/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* panel_domain_fit - panel-owned data-domain fit with equal aspect.
 *
 * Scenario: feature.panel_domain_fit
 * Style: features, graphite_cyan, 1600x1200 capture target
 *
 * Build:  just example-c features/panel_domain_fit
 * Run:    ./build/examples/c/features/panel_domain_fit --live
 * Smoke:  ./build/examples/c/features/panel_domain_fit --png
 */



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>

#include "_assertions.h"
#include "datoviz/scene.h"
#include "example_style.h"
#include "runner/scenario_runner.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH       1600u
#define HEIGHT      1200u
#define PATH_COUNT  5u
#define POINT_COUNT 4u



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Add a data-space rectangle and corner points.
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

    const vec3 data_path[PATH_COUNT] = {
        {0.0f, 0.0f, 0.0f}, {2.0f, 0.0f, 0.0f}, {2.0f, 1.0f, 0.0f},
        {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 0.0f},
    };
    DvzColor path_colors[PATH_COUNT] = {{0}};
    float widths[PATH_COUNT] = {0};
    for (uint32_t i = 0; i < PATH_COUNT; i++)
    {
        path_colors[i] = color;
        widths[i] = 5.0f;
    }

    DvzVisual* path = dvz_path(scene, 0);
    if (path == NULL)
        return false;
    DvzVisualDataUpdate path_updates[] = {
        {.attr_name = "position", .data = data_path, .item_count = PATH_COUNT},
        {.attr_name = "color", .data = path_colors, .item_count = PATH_COUNT},
        {.attr_name = "stroke_width", .data = widths, .item_count = PATH_COUNT},
    };
    if (dvz_visual_set_data_many(path, path_updates, 3) != 0)
        return false;
    if (dvz_path_set_caps(path, DVZ_SEGMENT_CAP_ROUND, DVZ_SEGMENT_CAP_ROUND) != 0)
        return false;
    if (dvz_path_set_join(path, DVZ_PATH_JOIN_ROUND, 4.0f) != 0)
        return false;
    DvzVisualAttachDesc attach = {DVZ_STRUCT_INIT_FIELDS(DvzVisualAttachDesc),
        .coord_space = DVZ_COORD_DATA};
    if (dvz_panel_add_visual(panel, path, &attach) != 0)
        return false;

    const vec3 data_points[POINT_COUNT] = {
        {0.0f, 0.0f, 0.0f},
        {2.0f, 0.0f, 0.0f},
        {2.0f, 1.0f, 0.0f},
        {0.0f, 1.0f, 0.0f},
    };
    DvzColor point_colors[POINT_COUNT] = {{0}};
    float diameters[POINT_COUNT] = {26.0f, 26.0f, 26.0f, 26.0f};
    for (uint32_t i = 0; i < POINT_COUNT; i++)
        point_colors[i] = example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_WARNING);

    DvzVisual* point = dvz_point(scene, 0);
    if (point == NULL)
        return false;
    DvzVisualDataUpdate point_updates[] = {
        {.attr_name = "position", .data = data_points, .item_count = POINT_COUNT},
        {.attr_name = "color", .data = point_colors, .item_count = POINT_COUNT},
        {.attr_name = "diameter", .data = diameters, .item_count = POINT_COUNT},
    };
    if (dvz_visual_set_data_many(point, point_updates, 3) != 0)
        return false;
    return dvz_panel_add_visual(panel, point, &attach) == 0;
}



/**
 * Configure axes for the panel-domain fit example.
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
    if (!dvz_axis_set_grid(x_axis, true) || !dvz_axis_set_grid(y_axis, true))
        return false;
    return dvz_axis_set_label(x_axis, "domain x") && dvz_axis_set_label(y_axis, "domain y");
}



/*************************************************************************************************/
/*  Scenario callbacks                                                                           */
/*************************************************************************************************/

/**
 * Initialize the panel-domain fit feature example.
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
    if (!dvz_grid_set_margins(
            grid, &(DvzPanelReserve){.left_px = 46.0f, .right_px = 46.0f, .top_px = 46.0f,
                                     .bottom_px = 58.0f}))
        return false;
    if (!dvz_grid_set_gutter(grid, 34.0f, 0.0f))
        return false;

    DvzPanel* free_panel = dvz_grid_panel(grid, 0, 0);
    DvzPanel* fit_panel = dvz_grid_panel(grid, 0, 1);
    if (free_panel == NULL || fit_panel == NULL)
        return false;

    DvzPanel* panels[2] = {free_panel, fit_panel};
    for (uint32_t i = 0; i < 2u; i++)
    {
        example_graphite_cyan_set_panel_background(panels[i]);
        if (!dvz_panel_set_layout_reserve(
                panels[i], &(DvzPanelLayoutReserve){.left = 0.13f, .right = 0.05f,
                                                    .bottom = 0.15f, .top = 0.07f}))
            return false;
        if (!_add_axes(panels[i]))
            return false;
    }

    if (dvz_panel_set_domain(free_panel, DVZ_DIM_X, 0.0, 2.0) != 0)
        return false;
    if (dvz_panel_set_domain(free_panel, DVZ_DIM_Y, 0.0, 1.0) != 0)
        return false;

    DvzPanelDomainFit fit = dvz_panel_domain_fit();
    fit.x = (DvzDataDomain){.min = 0.0, .max = 2.0};
    fit.y = (DvzDataDomain){.min = 0.0, .max = 1.0};
    fit.padding = 0.08;
    fit.aspect = DVZ_PANEL_DOMAIN_ASPECT_EQUAL;
    if (dvz_panel_set_domain_fit(fit_panel, &fit) != 0)
        return false;

    double x_min = 0.0;
    double x_max = 0.0;
    double y_min = 0.0;
    double y_max = 0.0;
    if (!dvz_panel_visible_domain(fit_panel, DVZ_DIM_X, &x_min, &x_max))
        return false;
    if (!dvz_panel_visible_domain(fit_panel, DVZ_DIM_Y, &y_min, &y_max))
        return false;
    if (!(x_min < 0.0 && x_max > 2.0 && y_min < 0.0 && y_max > 1.0))
        return false;

    if (!_add_domain_shape(
            ctx->scene, free_panel,
            example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ACCENT_SECONDARY)))
        return false;
    if (!_add_domain_shape(
            ctx->scene, fit_panel,
            example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ACCENT_PRIMARY)))
        return false;
    return dvz_scenario_panzoom(ctx, free_panel, NULL, DVZ_DIM_MASK_XY) != NULL &&
           dvz_scenario_panzoom(ctx, fit_panel, NULL, DVZ_DIM_MASK_XY) != NULL;
}



/**
 * Return the panel-domain fit scenario specification.
 *
 * @return scenario specification
 */
static DvzScenarioSpec _panel_domain_fit_scenario(void)
{
    return (DvzScenarioSpec){
        .id = "feature_panel_domain_fit",
        .title = "panel_domain_fit",
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
 * Run the panel-domain fit feature example through the native scenario runner.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @return process exit code
 */
int main(int argc, char** argv)
{
    DvzScenarioSpec spec = _panel_domain_fit_scenario();
    return dvz_scenario_run_native_cli(&spec, argc, argv) == 0 ? 0 : 1;
}
