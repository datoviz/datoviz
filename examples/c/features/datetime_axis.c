/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* datetime_axis - compact data coordinates with retained UTC datetime labels.
 *
 * Scenario: datetime_axis
 * Style: features, graphite_cyan, 1280x720 window target
 *
 * Build:  just example-c features/datetime_axis
 * Run:    ./build/examples/c/features/datetime_axis --live
 * Smoke:  ./build/examples/c/features/datetime_axis --png
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
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH  EXAMPLE_WINDOW_WIDTH
#define HEIGHT EXAMPLE_WINDOW_HEIGHT
#define SAMPLE_COUNT 320u

static const float TAU = 6.28318530718f;



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Fill one deterministic signal in compact data coordinates.
 *
 * @param positions output data-space positions
 * @param colors output colors
 * @param widths output stroke widths
 * @param count sample count
 */
static void _fill_signal(vec3* positions, DvzColor* colors, float* widths, uint32_t count)
{
    ANN(positions);
    ANN(colors);
    ANN(widths);

    DvzColor accent = example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ACCENT_PRIMARY);
    const float inv_count = count > 1u ? 1.0f / (float)(count - 1u) : 1.0f;
    for (uint32_t i = 0; i < count; i++)
    {
        const float t = (float)i * inv_count;
        positions[i][0] = 8.0f * t;
        positions[i][1] =
            0.46f * sinf(3.0f * TAU * t) + 0.18f * cosf(9.0f * TAU * t + 0.3f);
        positions[i][2] = 0.0f;
        colors[i] = accent;
        colors[i].a = 235u;
        widths[i] = 4.0f;
    }
}


/**
 * Upload one path visual.
 *
 * @param visual path visual
 * @param positions visual-space positions
 * @param colors path colors
 * @param widths stroke widths
 * @param count sample count
 * @return true when all uploads succeed
 */
static bool _upload_path(
    DvzVisual* visual, vec3* positions, DvzColor* colors, float* widths, uint32_t count)
{
    ANN(visual);
    ANN(positions);
    ANN(colors);
    ANN(widths);

    DvzVisualDataUpdate updates[] = {
        {.attr_name = "position", .data = positions, .item_count = count},
        {.attr_name = "color", .data = colors, .item_count = count},
        {.attr_name = "stroke_width_px", .data = widths, .item_count = count},
    };
    if (dvz_visual_set_data_many(visual, updates, 3) != 0)
        return false;
    if (dvz_path_set_caps(visual, DVZ_SEGMENT_CAP_ROUND, DVZ_SEGMENT_CAP_ROUND) != 0)
        return false;
    return dvz_path_set_join(visual, DVZ_PATH_JOIN_ROUND, 4.0f) == 0;
}



/*************************************************************************************************/
/*  Scenario callbacks                                                                           */
/*************************************************************************************************/

/**
 * Initialize the retained datetime axis feature scenario.
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

    vec3 data_positions[SAMPLE_COUNT] = {{0}};
    DvzColor colors[SAMPLE_COUNT] = {{0}};
    float widths[SAMPLE_COUNT] = {0};

    _fill_signal(data_positions, colors, widths, SAMPLE_COUNT);

    ctx->figure = dvz_figure(ctx->scene, ctx->width, ctx->height, 0);
    if (ctx->figure == NULL)
        return false;

    DvzPanel* panel = dvz_panel_full(ctx->figure);
    if (panel == NULL)
        return false;
    example_graphite_cyan_set_panel_background(panel);

    int rc = dvz_panel_set_domain(panel, DVZ_DIM_X, 0.0, 8.0);
    if (rc != 0)
        return false;
    rc = dvz_panel_set_domain(panel, DVZ_DIM_Y, -0.9, 0.9);
    if (rc != 0)
        return false;

    DvzVisual* path = dvz_path(ctx->scene, 0);
    if (path == NULL)
        return false;
    bool ok = _upload_path(path, data_positions, colors, widths, SAMPLE_COUNT);
    if (!ok)
        return false;
    rc = dvz_panel_add_visual(panel, path, NULL);
    if (rc != 0)
        return false;

    DvzAxis* x_axis = dvz_panel_axis(panel, DVZ_DIM_X);
    DvzAxis* y_axis = dvz_panel_axis(panel, DVZ_DIM_Y);
    if (x_axis == NULL || y_axis == NULL)
        return false;

    DvzAxisTickPolicy ticks = dvz_axis_tick_policy();
    ticks.target_count = 7;
    ticks.min_pixel_spacing = 130.0f;
    ticks.minor_per_interval = 3;
    ok = dvz_axis_set_tick_policy(x_axis, &ticks) == DVZ_OK;
    if (!ok)
        return false;
    ok = dvz_axis_set_tick_policy(y_axis, &ticks) == DVZ_OK;
    if (!ok)
        return false;

    ok = example_graphite_cyan_apply_axis_style(x_axis, false, NULL);
    if (!ok)
        return false;
    ok = example_graphite_cyan_apply_axis_style(y_axis, true, NULL);
    if (!ok)
        return false;
    ok = dvz_axis_set_grid(x_axis, true) == DVZ_OK;
    if (!ok)
        return false;
    ok = dvz_axis_set_grid(y_axis, true) == DVZ_OK;
    if (!ok)
        return false;

    DvzDateTimeFormat* datetime = dvz_datetime_format_create(ctx->scene);
    if (datetime == NULL)
        return false;
    rc = dvz_datetime_format_timezone(datetime, "UTC");
    if (rc != 0)
        return false;
    rc = dvz_datetime_format_rule(
        datetime, DVZ_TIME_INTERVAL_MICROSECOND, "%H:%M:%S.fff");
    if (rc != 0)
        return false;
    rc = dvz_datetime_format_rule(
        datetime, DVZ_TIME_INTERVAL_MILLISECOND, "%H:%M:%S.fff");
    if (rc != 0)
        return false;
    rc = dvz_datetime_format_rule(datetime, DVZ_TIME_INTERVAL_SECOND, "%H:%M:%S");
    if (rc != 0)
        return false;
    rc = dvz_datetime_format_rule(datetime, DVZ_TIME_INTERVAL_MINUTE, "%H:%M");
    if (rc != 0)
        return false;
    rc = dvz_datetime_format_rule(datetime, DVZ_TIME_INTERVAL_HOUR, "%H:%M");
    if (rc != 0)
        return false;
    rc = dvz_datetime_format_rule(datetime, DVZ_TIME_INTERVAL_DAY, "%b %d");
    if (rc != 0)
        return false;
    rc = dvz_datetime_format_rule(datetime, DVZ_TIME_INTERVAL_MONTH, "%Y-%m");
    if (rc != 0)
        return false;
    rc = dvz_datetime_format_rule(datetime, DVZ_TIME_INTERVAL_YEAR, "%Y");
    if (rc != 0)
        return false;
    const DvzTimestamp may_1_utc = (DvzTimestamp)1714554000000000LL; /* 2024-05-01 09:00 UTC */
    ok = dvz_axis_set_datetime(x_axis, datetime) == DVZ_OK;
    if (!ok)
        return false;
    ok = dvz_axis_set_datetime_range(
             x_axis, 0.0, 8.0, may_1_utc, may_1_utc + 8LL * 3600LL * 1000000LL) == DVZ_OK;
    if (!ok)
        return false;

    ok = dvz_axis_set_label(x_axis, "UTC time") == DVZ_OK;
    if (!ok)
        return false;
    ok = dvz_axis_set_label(y_axis, "signal") == DVZ_OK;
    if (!ok)
        return false;

    DvzPanzoom* panzoom = dvz_scenario_panzoom(ctx, panel, NULL, DVZ_DIM_MASK_X);
    return panzoom != NULL;
}



/**
 * Return the datetime-axis scenario specification.
 *
 * @return scenario specification
 */
static DvzScenarioSpec _datetime_axis_scenario(void)
{
    return (DvzScenarioSpec){
        .id = "datetime_axis",
        .title = "Datetime Axis",
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
 * Run the retained datetime axis feature proof through the native scenario runner.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @return process exit code
 */
int main(int argc, char** argv)
{
    DvzScenarioSpec spec = _datetime_axis_scenario();
    return dvz_scenario_run_native_cli(&spec, argc, argv) == 0 ? 0 : 1;
}
