/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* bars_bands - retained bars and uncertainty band plot helpers.
 *
 * Scenario: feature.bars_bands
 * Style: features, graphite_cyan, 1600x1200 capture target
 *
 * Build:  just example-c features/bars_bands
 * Run:    ./build/examples/c/features/bars_bands --live
 * Smoke:  ./build/examples/c/features/bars_bands --png
 */



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <math.h>
#include <stdbool.h>
#include <stdint.h>

#include "_assertions.h"
#include "datoviz/scene.h"
#include "example_style.h"
#include "runner/scenario_runner.h"



/*************************************************************************************************/
/*  Forward declarations                                                                         */
/*************************************************************************************************/

DvzScenarioSpec dvz_example_bars_bands_scenario(void);



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH      1600u
#define HEIGHT     1200u
#define BAR_COUNT  9u
#define BAND_COUNT 96u

static const double TAU = 6.2831853071795864769;



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Configure a compact 2D plotting panel.
 *
 * @param panel target panel
 * @return true when axes and domains are configured
 */
static bool _configure_panel(DvzPanel* panel)
{
    ANN(panel);

    example_graphite_cyan_set_panel_background(panel);
    if (dvz_panel_set_domain(panel, DVZ_DIM_X, -0.5, 8.5) != 0)
        return false;
    if (dvz_panel_set_domain(panel, DVZ_DIM_Y, -0.35, 2.25) != 0)
        return false;

    DvzAxis* x_axis = dvz_panel_axis(panel, DVZ_DIM_X);
    DvzAxis* y_axis = dvz_panel_axis(panel, DVZ_DIM_Y);
    if (x_axis == NULL || y_axis == NULL)
        return false;
    if (!example_graphite_cyan_apply_axis_style(x_axis, false, NULL))
        return false;
    if (!example_graphite_cyan_apply_axis_style(y_axis, true, NULL))
        return false;
    if (!dvz_axis_set_grid(x_axis, false) || !dvz_axis_set_grid(y_axis, false))
        return false;
    return dvz_axis_set_label(x_axis, "sample") && dvz_axis_set_label(y_axis, "value");
}



/**
 * Add explicit-interval bars.
 *
 * @param panel target panel
 * @return true when the bars were added
 */
static bool _add_bars(DvzPanel* panel)
{
    ANN(panel);

    double starts[BAR_COUNT] = {0};
    double ends[BAR_COUNT] = {0};
    double values[BAR_COUNT] = {0};
    for (uint32_t i = 0; i < BAR_COUNT; i++)
    {
        starts[i] = (double)i - 0.42;
        ends[i] = (double)i + 0.42;
        values[i] = 0.42 + 0.12 * (double)i + 0.32 * sin(0.70 * (double)i);
    }

    DvzBarsDesc desc = dvz_bars_desc();
    desc.fill_color = dvz_color_rgba(76, 201, 240, 150);
    desc.outline_color = dvz_color_rgba(76, 201, 240, 95);
    desc.outline_width_px = 1.0f;
    desc.gap_fraction = 0.12f;

    DvzBars* bars = dvz_bars(panel, &desc);
    return bars != NULL && dvz_bars_set_intervals(bars, BAR_COUNT, starts, ends, values) == 0;
}



/**
 * Add a continuous band with lower/upper bounds and a center line.
 *
 * @param panel target panel
 * @return true when the band was added
 */
static bool _add_band(DvzPanel* panel)
{
    ANN(panel);

    double x[BAND_COUNT] = {0};
    double lower[BAND_COUNT] = {0};
    double upper[BAND_COUNT] = {0};
    double center[BAND_COUNT] = {0};

    for (uint32_t i = 0; i < BAND_COUNT; i++)
    {
        const double t = (double)i / (double)(BAND_COUNT - 1u);
        x[i] = 8.0 * t;
        center[i] = 0.74 + 0.48 * t + 0.22 * sin(TAU * (1.35 * t + 0.08));
        const double half_width = 0.18 + 0.07 * cos(TAU * t);
        lower[i] = center[i] - half_width;
        upper[i] = center[i] + half_width;
    }

    DvzBandDesc desc = dvz_band_desc();
    desc.fill_color = dvz_color_rgba(128, 255, 219, 58);
    desc.line_color = dvz_color_rgba(128, 255, 219, 255);
    desc.line_width_px = 5.0f;
    desc.show_bounds = true;
    desc.bound_color = dvz_color_rgba(128, 255, 219, 150);
    desc.bound_width_px = 1.5f;

    DvzBand* band = dvz_band(panel, &desc);
    return band != NULL && dvz_band_set_bounds(band, BAND_COUNT, x, lower, upper) == 0 &&
           dvz_band_set_center(band, BAND_COUNT, x, center) == 0;
}



/*************************************************************************************************/
/*  Scenario callbacks                                                                           */
/*************************************************************************************************/

/**
 * Initialize the bars/bands feature example.
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

    if (!_configure_panel(panel) || !_add_bars(panel) || !_add_band(panel))
        return false;
    return dvz_scenario_panzoom(ctx, panel, NULL, DVZ_DIM_MASK_XY) != NULL;
}



/**
 * Return the bars/bands scenario specification.
 *
 * @return scenario specification
 */
DvzScenarioSpec dvz_example_bars_bands_scenario(void)
{
    return (DvzScenarioSpec){
        .id = "feature_bars_bands",
        .title = "bars_bands",
        .width = WIDTH,
        .height = HEIGHT,
        .fps = 60.0,
        .requirements = DVZ_SCENARIO_REQ_CONTROLLER | DVZ_SCENARIO_REQ_PANZOOM,
        .init = _scenario_init,
    };
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Run the bars/bands feature example through the native scenario runner.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @return process exit code
 */
#ifndef DVZ_EXAMPLE_NO_MAIN
int main(int argc, char** argv)
{
    DvzScenarioSpec spec = dvz_example_bars_bands_scenario();
    return dvz_scenario_run_native_cli(&spec, argc, argv) == 0 ? 0 : 1;
}
#endif
