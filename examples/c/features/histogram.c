/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* histogram - This example bins deterministic samples and renders a histogram with DvzBars.
 *
 * Scenario: features_histogram
 * Style: features, graphite_cyan, 1280x720 window target
 *
 * Build:  just example-c features/histogram
 * Run:    ./build/examples/c/features/histogram --live
 * Smoke:  ./build/examples/c/features/histogram --png
 *
 * What to look for: the example computes bin counts on the CPU, then gives dvz_bars explicit
 * left edges, right edges, and counts. Datoviz renders the pre-binned values; applications or
 * higher-level plotting libraries remain responsible for choosing a statistical binning policy.
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

DvzScenarioSpec dvz_example_histogram_scenario(void);



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH        EXAMPLE_WINDOW_WIDTH
#define HEIGHT       EXAMPLE_WINDOW_HEIGHT
#define SAMPLE_COUNT 512u
#define BIN_COUNT    24u

static const double VALUE_MIN = -3.0;
static const double VALUE_MAX = +3.0;
static const double TAU = 6.2831853071795864769;



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Generate deterministic samples from a two-component distribution.
 *
 * @param samples output sample values
 */
static void _generate_samples(double samples[SAMPLE_COUNT])
{
    ANN(samples);

    for (uint32_t i = 0; i < SAMPLE_COUNT; i++)
    {
        const double t = (double)i / (double)SAMPLE_COUNT;
        const double carrier = sin(TAU * (17.0 * t + 0.13));
        const double detail = 0.42 * sin(TAU * (43.0 * t + 0.37));
        const double center = i < 310u ? -0.72 : 1.08;
        const double spread = i < 310u ? 0.92 : 0.58;
        samples[i] = center + spread * (carrier + detail);
    }
}



/**
 * Bin samples into explicit intervals suitable for DvzBars.
 *
 * @param samples input sample values
 * @param starts output left bin edges
 * @param ends output right bin edges
 * @param counts output bin counts
 * @return largest bin count
 */
static double _bin_samples(
    const double samples[SAMPLE_COUNT], double starts[BIN_COUNT], double ends[BIN_COUNT],
    double counts[BIN_COUNT])
{
    ANN(samples);
    ANN(starts);
    ANN(ends);
    ANN(counts);

    const double bin_width = (VALUE_MAX - VALUE_MIN) / (double)BIN_COUNT;
    for (uint32_t i = 0; i < BIN_COUNT; i++)
    {
        starts[i] = VALUE_MIN + (double)i * bin_width;
        ends[i] = starts[i] + bin_width;
        counts[i] = 0.0;
    }

    for (uint32_t i = 0; i < SAMPLE_COUNT; i++)
    {
        const double normalized = (samples[i] - VALUE_MIN) / (VALUE_MAX - VALUE_MIN);
        int64_t bin = (int64_t)floor(normalized * (double)BIN_COUNT);
        if (bin == (int64_t)BIN_COUNT && samples[i] == VALUE_MAX)
            bin = (int64_t)BIN_COUNT - 1;
        if (bin >= 0 && bin < (int64_t)BIN_COUNT)
            counts[bin] += 1.0;
    }

    double max_count = 0.0;
    for (uint32_t i = 0; i < BIN_COUNT; i++)
        max_count = counts[i] > max_count ? counts[i] : max_count;
    return max_count;
}



/**
 * Configure histogram domains and axes.
 *
 * @param panel target panel
 * @param max_count largest bin count
 * @return true when the panel is configured
 */
static bool _configure_panel(DvzPanel* panel, double max_count)
{
    ANN(panel);

    example_graphite_cyan_set_panel_background(panel);
    if (dvz_panel_set_domain(panel, DVZ_DIM_X, VALUE_MIN, VALUE_MAX) != DVZ_OK)
        return false;
    if (dvz_panel_set_domain(panel, DVZ_DIM_Y, 0.0, 1.15 * max_count) != DVZ_OK)
        return false;

    DvzAxis* x_axis = dvz_panel_axis(panel, DVZ_DIM_X);
    DvzAxis* y_axis = dvz_panel_axis(panel, DVZ_DIM_Y);
    if (x_axis == NULL || y_axis == NULL)
        return false;
    if (!example_graphite_cyan_apply_axis_style(x_axis, false, NULL))
        return false;
    if (!example_graphite_cyan_apply_axis_style(y_axis, true, NULL))
        return false;
    if (dvz_axis_set_grid(x_axis, false) != DVZ_OK || dvz_axis_set_grid(y_axis, true) != DVZ_OK)
        return false;
    return dvz_axis_set_label(x_axis, "value") == DVZ_OK &&
           dvz_axis_set_label(y_axis, "count") == DVZ_OK;
}



/*************************************************************************************************/
/*  Scenario callbacks                                                                           */
/*************************************************************************************************/

/**
 * Initialize the deterministic histogram feature example.
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

    double samples[SAMPLE_COUNT] = {0};
    double starts[BIN_COUNT] = {0};
    double ends[BIN_COUNT] = {0};
    double counts[BIN_COUNT] = {0};
    _generate_samples(samples);
    const double max_count = _bin_samples(samples, starts, ends, counts);

    ctx->figure = dvz_figure(ctx->scene, ctx->width, ctx->height, 0);
    if (ctx->figure == NULL)
        return false;

    DvzPanel* panel = dvz_panel_full(ctx->figure);
    if (panel == NULL || !_configure_panel(panel, max_count))
        return false;

    DvzBarsDesc desc = dvz_bars_desc();
    desc.fill_color = dvz_color_rgba(76, 201, 240, 190);
    desc.outline_color = dvz_color_rgba(128, 255, 219, 210);
    desc.outline_width_px = 1.25f;
    desc.gap_fraction = 0.08f;

    DvzBars* bars = dvz_bars(panel, &desc);
    if (bars == NULL || dvz_bars_set_intervals(bars, starts, ends, counts, BIN_COUNT) != DVZ_OK)
        return false;
    return dvz_scenario_panzoom(ctx, panel, NULL, DVZ_DIM_MASK_XY) != NULL;
}



/**
 * Return the histogram scenario specification.
 *
 * @return scenario specification
 */
DvzScenarioSpec dvz_example_histogram_scenario(void)
{
    return (DvzScenarioSpec){
        .id = "features_histogram",
        .title = "Histogram",
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
 * Run the histogram feature example through the native scenario runner.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @return process exit code
 */
#ifndef DVZ_EXAMPLE_NO_MAIN
int main(int argc, char** argv)
{
    DvzScenarioSpec spec = dvz_example_histogram_scenario();
    return dvz_scenario_run_native_cli(&spec, argc, argv) == 0 ? 0 : 1;
}
#endif
