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
 * Run:    ./build/examples/c/features/axis_labels --live
 * Smoke:  ./build/examples/c/features/axis_labels --png
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

#define WIDTH         1600u
#define HEIGHT        1200u
#define SEGMENT_COUNT 4u



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
    const float bottom = -0.62f;
    const float top = +0.68f;

    vec3 starts[SEGMENT_COUNT] = {
        {left, bottom, 0.0f},
        {right, bottom, 0.0f},
        {right, top, 0.0f},
        {left, top, 0.0f},
    };
    vec3 ends[SEGMENT_COUNT] = {
        {right, bottom, 0.0f},
        {right, top, 0.0f},
        {left, top, 0.0f},
        {left, bottom, 0.0f},
    };
    DvzColor colors[SEGMENT_COUNT] = {0};
    float widths[SEGMENT_COUNT] = {0};

    for (uint32_t i = 0; i < SEGMENT_COUNT; i++)
    {
        colors[i] = example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ACCENT_PRIMARY);
        colors[i].a = 180u;
        widths[i] = 2.5f;
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
/*  Scenario callbacks                                                                           */
/*************************************************************************************************/

/**
 * Initialize the axis label placement scenario.
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

    DvzPanel* panel = dvz_panel_full(ctx->figure);
    if (panel == NULL)
        return false;
    example_graphite_cyan_set_panel_background(panel);

    bool ok = dvz_panel_set_layout_reserve(
        panel, &(DvzPanelLayoutReserve){.left = 0.14f, .right = 0.08f, .bottom = 0.15f,
                                        .top = 0.08f});
    if (!ok)
        return false;

    if (dvz_panel_set_domain(panel, DVZ_DIM_X, -40.0, 120.0) != 0)
        return false;
    if (dvz_panel_set_domain(panel, DVZ_DIM_Y, -1.5, 2.5) != 0)
        return false;
    if (!_add_plot_guides(ctx->scene, panel))
        return false;

    DvzAxis* x_axis = dvz_panel_axis(panel, DVZ_DIM_X);
    DvzAxis* y_axis = dvz_panel_axis(panel, DVZ_DIM_Y);
    if (x_axis == NULL || y_axis == NULL)
        return false;

    DvzAxisTickPolicy ticks = dvz_axis_tick_policy();
    ticks.target_count = 5;
    ticks.min_pixel_spacing = 150.0f;
    ticks.minor_per_interval = 2;
    if (!dvz_axis_set_tick_policy(x_axis, &ticks))
        return false;
    if (!dvz_axis_set_tick_policy(y_axis, &ticks))
        return false;

    ExampleAxisStyleOptions style = example_graphite_cyan_axis_options();
    style.tick_size_px = 14.0f;
    style.label_size_px = 20.0f;
    style.tick_gap_px = 10.0f;
    style.x_label_gap_px = 48.0f;
    style.y_label_gap_px = 70.0f;
    style.grid_alpha = 130u;
    if (!example_graphite_cyan_apply_axis_style(x_axis, false, &style))
        return false;
    if (!example_graphite_cyan_apply_axis_style(y_axis, true, &style))
        return false;

    if (!dvz_axis_set_plot_margins(x_axis, 0.18f, 0.12f, 0.18f, 0.12f))
        return false;
    if (!dvz_axis_set_plot_margins(y_axis, 0.18f, 0.12f, 0.18f, 0.12f))
        return false;
    if (!dvz_axis_set_grid(x_axis, true))
        return false;
    if (!dvz_axis_set_grid(y_axis, true))
        return false;
    if (!dvz_axis_set_label(x_axis, "sample offset (ms)"))
        return false;
    if (!dvz_axis_set_label(y_axis, "normalized response"))
        return false;

    return true;
}



/**
 * Return the axis-labels scenario specification.
 *
 * @return scenario specification
 */
static DvzScenarioSpec _axis_labels_scenario(void)
{
    return (DvzScenarioSpec){
        .id = "feature_axis_labels",
        .title = "axis_labels",
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
 * Run the axis label placement feature example through the native scenario runner.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @return process exit code
 */
int main(int argc, char** argv)
{
    DvzScenarioSpec spec = _axis_labels_scenario();
    return dvz_scenario_run_native_cli(&spec, argc, argv) == 0 ? 0 : 1;
}
