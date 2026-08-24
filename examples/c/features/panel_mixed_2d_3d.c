/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* panel_mixed_2d_3d - This example mixes native Datoviz 2D and 3D panels in one figure.
 *
 * What to look for: the left panel spans both grid rows and shows a Lorenz trajectory with an
 * arcball controller. The right panels show x(t) and z(t) from the same samples, each with retained
 * axes and an independent panzoom controller. This is the all-Datoviz counterpart to the optional
 * advanced/gui_implot example, which keeps the Datoviz 3D view but draws the two charts with
 * ImPlot.
 *
 * Scenario: features_panel_mixed_2d_3d
 * Style: features, graphite_cyan, 1280x720 window target
 *
 * Build:  just example-c features/panel_mixed_2d_3d
 * Run:    ./build/examples/c/features/panel_mixed_2d_3d --live
 * Smoke:  ./build/examples/c/features/panel_mixed_2d_3d --png
 */



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>

#include "_assertions.h"
#include "datoviz/scene.h"
#include "example_common.h"
#include "example_style.h"
#include "runner/scenario_runner.h"



/*************************************************************************************************/
/*  Forward declarations                                                                         */
/*************************************************************************************************/

DvzScenarioSpec dvz_example_panel_mixed_2d_3d_scenario(void);



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH  EXAMPLE_WINDOW_WIDTH
#define HEIGHT EXAMPLE_WINDOW_HEIGHT
#define SAMPLE_COUNT 1600u
#define WARMUP_COUNT 1200u

static const double LORENZ_DT = 0.005;



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Advance one Lorenz-system sample with a small explicit integration step.
 *
 * @param x mutable X state
 * @param y mutable Y state
 * @param z mutable Z state
 */
static void _lorenz_step(double* x, double* y, double* z)
{
    ANN(x);
    ANN(y);
    ANN(z);

    const double dx = 10.0 * (*y - *x);
    const double dy = *x * (28.0 - *z) - *y;
    const double dz = *x * *y - (8.0 / 3.0) * *z;
    *x += LORENZ_DT * dx;
    *y += LORENZ_DT * dy;
    *z += LORENZ_DT * dz;
}



/**
 * Generate one normalized 3D trajectory and its two data-space traces.
 *
 * @param trajectory output normalized XYZ positions
 * @param x_trace output time/X positions
 * @param z_trace output time/Z positions
 */
static void _fill_lorenz(vec3* trajectory, vec3* x_trace, vec3* z_trace)
{
    ANN(trajectory);
    ANN(x_trace);
    ANN(z_trace);

    double x = 0.1;
    double y = 0.0;
    double z = 0.0;
    for (uint32_t i = 0; i < WARMUP_COUNT; i++)
        _lorenz_step(&x, &y, &z);

    for (uint32_t i = 0; i < SAMPLE_COUNT; i++)
    {
        _lorenz_step(&x, &y, &z);
        const float t = (float)((double)i * LORENZ_DT);

        trajectory[i][0] = (float)(x / 22.0);
        trajectory[i][1] = (float)((z - 25.0) / 28.0);
        trajectory[i][2] = (float)(y / 30.0);
        x_trace[i][0] = t;
        x_trace[i][1] = (float)x;
        z_trace[i][0] = t;
        z_trace[i][1] = (float)z;
    }
}



/**
 * Configure shared panel chrome.
 *
 * @param panel target panel
 * @return true on success
 */
static bool _configure_panel(DvzPanel* panel)
{
    ANN(panel);

    example_graphite_cyan_set_panel_background(panel);
    DvzPanelBorderDesc border = dvz_panel_border_desc();
    border.color = example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_GRID);
    border.width_px = 1.5f;
    return dvz_panel_set_border(panel, &border) == DVZ_OK;
}



/**
 * Create and attach one stroked path.
 *
 * @param scene scene owning the visual
 * @param panel panel receiving the visual
 * @param positions path positions
 * @param color uniform path color
 * @param data_coordinates whether positions use the panel data domain
 * @return true on success
 */
static bool _add_path(
    DvzScene* scene, DvzPanel* panel, const vec3* positions, DvzColor color,
    bool data_coordinates)
{
    ANN(scene);
    ANN(panel);
    ANN(positions);

    DvzColor colors[SAMPLE_COUNT] = {{0}};
    float widths[SAMPLE_COUNT] = {0};
    for (uint32_t i = 0; i < SAMPLE_COUNT; i++)
    {
        colors[i] = color;
        widths[i] = data_coordinates ? 3.0f : 2.4f;
    }

    DvzVisual* path = dvz_path(scene, 0);
    if (path == NULL)
        return false;
    DvzVisualDataUpdate updates[] = {
        {.attr_name = "position", .data = positions, .item_count = SAMPLE_COUNT},
        {.attr_name = "color", .data = colors, .item_count = SAMPLE_COUNT},
        {.attr_name = "stroke_width_px", .data = widths, .item_count = SAMPLE_COUNT},
    };
    if (dvz_visual_set_data_many(path, updates, 3) != DVZ_OK)
        return false;
    if (dvz_path_set_caps(path, DVZ_SEGMENT_CAP_ROUND, DVZ_SEGMENT_CAP_ROUND) != DVZ_OK)
        return false;
    if (dvz_path_set_join(path, DVZ_PATH_JOIN_ROUND, 4.0f) != DVZ_OK)
        return false;
    if (dvz_visual_set_depth_test(path, !data_coordinates) != DVZ_OK)
        return false;

    DvzVisualAttachDesc attach = dvz_visual_attach_desc();
    if (data_coordinates)
        attach.coord_space = DVZ_VISUAL_COORD_DATA;
    return dvz_panel_add_visual(panel, path, data_coordinates ? &attach : NULL) == DVZ_OK;
}



/**
 * Configure one retained 2D chart panel.
 *
 * @param panel target panel
 * @param y_min Y domain minimum
 * @param y_max Y domain maximum
 * @param y_label Y axis label
 * @return true on success
 */
static bool _configure_chart(DvzPanel* panel, double y_min, double y_max, const char* y_label)
{
    ANN(panel);
    ANN(y_label);

    const double t_max = (double)(SAMPLE_COUNT - 1u) * LORENZ_DT;
    if (dvz_panel_set_domain(panel, DVZ_DIM_X, 0.0, t_max) != DVZ_OK)
        return false;
    if (dvz_panel_set_domain(panel, DVZ_DIM_Y, y_min, y_max) != DVZ_OK)
        return false;
    if (dvz_panel_set_reserve(
            panel,
            &(DvzPanelReserve){
                .left_px = 64.0f, .right_px = 18.0f, .top_px = 18.0f, .bottom_px = 48.0f}) !=
        DVZ_OK)
        return false;

    DvzAxis* x_axis = dvz_panel_axis(panel, DVZ_DIM_X);
    DvzAxis* y_axis = dvz_panel_axis(panel, DVZ_DIM_Y);
    if (x_axis == NULL || y_axis == NULL)
        return false;
    if (!example_graphite_cyan_apply_axis_style(x_axis, false, NULL))
        return false;
    if (!example_graphite_cyan_apply_axis_style(y_axis, true, NULL))
        return false;
    if (dvz_axis_set_grid(x_axis, true) != DVZ_OK || dvz_axis_set_grid(y_axis, true) != DVZ_OK)
        return false;
    if (dvz_axis_set_label(x_axis, "time (s)") != DVZ_OK)
        return false;
    return dvz_axis_set_label(y_axis, y_label) == DVZ_OK;
}



/**
 * Add a screen-space label to the 3D panel.
 *
 * @param panel target panel
 * @return true on success
 */
static bool _add_3d_label(DvzPanel* panel)
{
    ANN(panel);

    DvzLabelDesc desc = dvz_label_desc();
    desc.text = "Datoviz 3D trajectory";
    DvzTextStyle style = example_graphite_cyan_text_style(EXAMPLE_STYLE_TEXT_PANEL_LABEL);
    style.size_px = 18.0f;
    DvzTextPlacement placement = dvz_text_placement();
    placement.mode = DVZ_TEXT_PLACEMENT_SCREEN;
    placement.anchor = DVZ_SCENE_ANCHOR_PANEL_TOP_LEFT;
    placement.position[0] = 20.0f;
    placement.position[1] = 20.0f;
    placement.has_text_anchor = true;

    DvzAnnotation* annotation = dvz_annotation_label(panel, &desc);
    return annotation != NULL && dvz_annotation_set_style(annotation, &style) == DVZ_OK &&
           dvz_annotation_set_placement(annotation, &placement) == DVZ_OK;
}



/*************************************************************************************************/
/*  Scenario callbacks                                                                           */
/*************************************************************************************************/

/**
 * Initialize the mixed native 2D/3D panel scenario.
 *
 * @param ctx scenario context
 * @param out_user unused scenario state output
 * @return true on success
 */
static bool _scenario_init(DvzScenarioContext* ctx, void** out_user)
{
    if (ctx == NULL)
        return false;
    if (out_user != NULL)
        *out_user = NULL;

    vec3 trajectory[SAMPLE_COUNT] = {{0}};
    vec3 x_trace[SAMPLE_COUNT] = {{0}};
    vec3 z_trace[SAMPLE_COUNT] = {{0}};
    _fill_lorenz(trajectory, x_trace, z_trace);

    ctx->figure = dvz_figure(ctx->scene, ctx->width, ctx->height, 0);
    if (ctx->figure == NULL)
        return false;

    DvzGrid* grid = dvz_figure_grid(ctx->figure, 2, 2);
    if (grid == NULL)
        return false;
    if (dvz_grid_set_margins(
            grid,
            &(DvzPanelReserve){
                .left_px = 24.0f, .right_px = 24.0f, .top_px = 24.0f, .bottom_px = 24.0f}) !=
        DVZ_OK)
        return false;
    if (dvz_grid_set_gutter(grid, 18.0f, 18.0f) != DVZ_OK)
        return false;
    if (dvz_grid_set_col_size(grid, 0, DVZ_GRID_SIZE_WEIGHT, 1.55f) != DVZ_OK)
        return false;
    if (dvz_grid_set_col_size(grid, 1, DVZ_GRID_SIZE_WEIGHT, 1.0f) != DVZ_OK)
        return false;

    DvzPanel* trajectory_panel = dvz_grid_panel_span(grid, 0, 0, 2, 1);
    DvzPanel* x_panel = dvz_grid_panel(grid, 0, 1);
    DvzPanel* z_panel = dvz_grid_panel(grid, 1, 1);
    if (trajectory_panel == NULL || x_panel == NULL || z_panel == NULL)
        return false;
    if (!_configure_panel(trajectory_panel) || !_configure_panel(x_panel) ||
        !_configure_panel(z_panel))
        return false;

    DvzColor primary = example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ACCENT_PRIMARY);
    DvzColor secondary = example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ACCENT_SECONDARY);
    primary.a = 244u;
    secondary.a = 244u;
    if (!_add_path(ctx->scene, trajectory_panel, trajectory, primary, false))
        return false;
    if (!_add_path(ctx->scene, x_panel, x_trace, primary, true))
        return false;
    if (!_add_path(ctx->scene, z_panel, z_trace, secondary, true))
        return false;

    if (example_set_default_3d_camera(trajectory_panel, 1.15f) == NULL)
        return false;
    DvzReferenceGridDesc reference = dvz_reference_grid_desc();
    reference.plane = DVZ_REFERENCE_GRID_XZ;
    reference.origin[1] = -0.92f;
    reference.size[0] = 2.4f;
    reference.size[1] = 2.4f;
    reference.spacing = 0.2f;
    reference.major_every = 5u;
    if (dvz_reference_grid(trajectory_panel, &reference) == NULL)
        return false;
    if (!_add_3d_label(trajectory_panel))
        return false;
    if (!_configure_chart(x_panel, -22.0, 22.0, "x(t)"))
        return false;
    if (!_configure_chart(z_panel, 0.0, 52.0, "z(t)"))
        return false;

    DvzController* arcball = dvz_arcball(ctx->scene, NULL);
    if (arcball == NULL ||
        dvz_scenario_bind_controller(ctx, trajectory_panel, arcball, DVZ_DIM_MASK_XYZ) != DVZ_OK)
        return false;
    if (dvz_scenario_panzoom(ctx, x_panel, NULL, DVZ_DIM_MASK_XY) == NULL)
        return false;
    return dvz_scenario_panzoom(ctx, z_panel, NULL, DVZ_DIM_MASK_XY) != NULL;
}



/**
 * Return the mixed native 2D/3D panel scenario specification.
 *
 * @return scenario specification
 */
DvzScenarioSpec dvz_example_panel_mixed_2d_3d_scenario(void)
{
    return (DvzScenarioSpec){
        .id = "features_panel_mixed_2d_3d",
        .title = "Mixed 2D and 3D Panels",
        .width = WIDTH,
        .height = HEIGHT,
        .fps = 60.0,
        .requirements = DVZ_SCENARIO_REQ_TEXT_VISUAL | DVZ_SCENARIO_REQ_CONTROLLER |
                        DVZ_SCENARIO_REQ_PANZOOM | DVZ_SCENARIO_REQ_ARCBALL,
        .init = _scenario_init,
    };
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Run the mixed native 2D/3D panel example through the native scenario runner.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @return process exit code
 */
#ifndef DVZ_EXAMPLE_NO_MAIN
int main(int argc, char** argv)
{
    DvzScenarioSpec spec = dvz_example_panel_mixed_2d_3d_scenario();
    return dvz_scenario_run_native_cli(&spec, argc, argv) == DVZ_OK ? 0 : 1;
}
#endif
