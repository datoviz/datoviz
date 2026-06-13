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
/*  Forward declarations                                                                         */
/*************************************************************************************************/

DvzScenarioSpec dvz_example_axis_labels_scenario(void);



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH         1600u
#define HEIGHT        1200u
#define SEGMENT_COUNT 4u



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

    if (dvz_panel_set_domain(panel, DVZ_DIM_X, -40.0, 120.0) != 0)
        return false;
    if (dvz_panel_set_domain(panel, DVZ_DIM_Y, -1.5, 2.5) != 0)
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
    style.grid_alpha = 130u;
    if (!example_graphite_cyan_apply_axis_style(x_axis, false, &style))
        return false;
    if (!example_graphite_cyan_apply_axis_style(y_axis, true, &style))
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
DvzScenarioSpec dvz_example_axis_labels_scenario(void)
{
    return (DvzScenarioSpec){
        .id = "feature_axis_labels",
        .title = "axis_labels",
        .width = WIDTH,
        .height = HEIGHT,
        .fps = 60.0,
        .requirements = DVZ_SCENARIO_REQ_TEXT_VISUAL,
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
#ifndef DVZ_EXAMPLE_NO_MAIN
int main(int argc, char** argv)
{
    DvzScenarioSpec spec = dvz_example_axis_labels_scenario();
    return dvz_scenario_run_native_cli(&spec, argc, argv) == 0 ? 0 : 1;
}
#endif
