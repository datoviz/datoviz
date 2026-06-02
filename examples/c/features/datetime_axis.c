/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* datetime_axis - compact data coordinates with retained UTC datetime labels.
 *
 * Scenario: datetime_axis
 * Style: features, graphite_cyan, 1600x1200 capture target
 *
 * Build:  just example-c features/datetime_axis
 * Run:    ./build/examples/c/features/datetime_axis
 * Smoke:  ./build/examples/c/features/datetime_axis 1
 * PNG:    DVZ_CAPTURE=png ./build/examples/c/features/datetime_axis 1
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

#define WIDTH        1600u
#define HEIGHT       1200u
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
        {.attr_name = "stroke_width", .data = widths, .item_count = count},
    };
    if (dvz_visual_set_data_many(visual, updates, 3) != 0)
        return false;
    if (dvz_path_set_caps(visual, DVZ_SEGMENT_CAP_ROUND, DVZ_SEGMENT_CAP_ROUND) != 0)
        return false;
    return dvz_path_set_join(visual, DVZ_PATH_JOIN_ROUND, 4.0f) == 0;
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Run the retained datetime axis feature proof.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @return process exit code
 */
int main(int argc, char** argv)
{
    int ret = 1;
    DvzScene* scene = NULL;
    DvzApp* app = NULL;
    vec3 data_positions[SAMPLE_COUNT] = {{0}};
    vec3 visual_positions[SAMPLE_COUNT] = {{0}};
    DvzColor colors[SAMPLE_COUNT] = {{0}};
    float widths[SAMPLE_COUNT] = {0};

    _fill_signal(data_positions, colors, widths, SAMPLE_COUNT);

    scene = dvz_scene();
    EXAMPLE_CHECK(scene != NULL, "dvz_scene() failed");

    DvzFigure* figure = dvz_figure(scene, WIDTH, HEIGHT, 0);
    EXAMPLE_CHECK(figure != NULL, "dvz_figure() failed");

    DvzPanel* panel = dvz_panel_full(figure);
    EXAMPLE_CHECK(panel != NULL, "dvz_panel_full() failed");
    example_graphite_cyan_set_panel_background(panel);

    bool ok = dvz_panel_set_layout_reserve(
        panel, &(DvzPanelLayoutReserve){.left = 0.12f, .right = 0.06f, .bottom = 0.16f,
                                        .top = 0.06f});
    EXAMPLE_CHECK(ok, "dvz_panel_set_layout_reserve() failed");

    int rc = dvz_panel_set_domain(panel, DVZ_DIM_X, 0.0, 8.0);
    EXAMPLE_CHECK(rc == 0, "dvz_panel_set_domain(x) failed");
    rc = dvz_panel_set_domain(panel, DVZ_DIM_Y, -0.9, 0.9);
    EXAMPLE_CHECK(rc == 0, "dvz_panel_set_domain(y) failed");

    rc = dvz_panel_data_to_visual_positions(
        panel, (const float*)data_positions, (float*)visual_positions, SAMPLE_COUNT);
    EXAMPLE_CHECK(rc == 0, "dvz_panel_data_to_visual_positions() failed");

    DvzVisual* path = dvz_path(scene, 0);
    EXAMPLE_CHECK(path != NULL, "dvz_path() failed");
    ok = _upload_path(path, visual_positions, colors, widths, SAMPLE_COUNT);
    EXAMPLE_CHECK(ok, "path data upload failed");
    rc = dvz_panel_add_visual(panel, path, NULL);
    EXAMPLE_CHECK(rc == 0, "dvz_panel_add_visual() failed");

    DvzAxis* x_axis = dvz_panel_axis(panel, DVZ_DIM_X);
    DvzAxis* y_axis = dvz_panel_axis(panel, DVZ_DIM_Y);
    EXAMPLE_CHECK(x_axis != NULL && y_axis != NULL, "dvz_panel_axis() failed");

    DvzAxisTickPolicy ticks = dvz_axis_tick_policy();
    ticks.target_count = 7;
    ticks.min_pixel_spacing = 130.0f;
    ticks.minor_per_interval = 3;
    ok = dvz_axis_set_tick_policy(x_axis, &ticks);
    EXAMPLE_CHECK(ok, "dvz_axis_set_tick_policy() failed for X");
    ok = dvz_axis_set_tick_policy(y_axis, &ticks);
    EXAMPLE_CHECK(ok, "dvz_axis_set_tick_policy() failed for Y");

    ok = example_graphite_cyan_apply_axis_style(x_axis, false, NULL);
    EXAMPLE_CHECK(ok, "dvz_axis_set_style() failed for X");
    ok = example_graphite_cyan_apply_axis_style(y_axis, true, NULL);
    EXAMPLE_CHECK(ok, "dvz_axis_set_style() failed for Y");
    ok = dvz_axis_set_grid(x_axis, true);
    EXAMPLE_CHECK(ok, "dvz_axis_set_grid() failed for X");
    ok = dvz_axis_set_grid(y_axis, true);
    EXAMPLE_CHECK(ok, "dvz_axis_set_grid() failed for Y");

    DvzDateTimeFormat* datetime = dvz_datetime_format_create(scene);
    EXAMPLE_CHECK(datetime != NULL, "dvz_datetime_format_create() failed");
    rc = dvz_datetime_format_timezone(datetime, "UTC");
    EXAMPLE_CHECK(rc == 0, "dvz_datetime_format_timezone() failed");
    rc = dvz_datetime_format_rule(
        datetime, DVZ_TIME_INTERVAL_MICROSECOND, "%H:%M:%S.fff");
    EXAMPLE_CHECK(rc == 0, "dvz_datetime_format_rule(microsecond) failed");
    rc = dvz_datetime_format_rule(
        datetime, DVZ_TIME_INTERVAL_MILLISECOND, "%H:%M:%S.fff");
    EXAMPLE_CHECK(rc == 0, "dvz_datetime_format_rule(millisecond) failed");
    rc = dvz_datetime_format_rule(datetime, DVZ_TIME_INTERVAL_SECOND, "%H:%M:%S");
    EXAMPLE_CHECK(rc == 0, "dvz_datetime_format_rule(second) failed");
    rc = dvz_datetime_format_rule(datetime, DVZ_TIME_INTERVAL_MINUTE, "%H:%M");
    EXAMPLE_CHECK(rc == 0, "dvz_datetime_format_rule(minute) failed");
    rc = dvz_datetime_format_rule(datetime, DVZ_TIME_INTERVAL_HOUR, "%H:%M");
    EXAMPLE_CHECK(rc == 0, "dvz_datetime_format_rule(hour) failed");
    rc = dvz_datetime_format_rule(datetime, DVZ_TIME_INTERVAL_DAY, "%b %d");
    EXAMPLE_CHECK(rc == 0, "dvz_datetime_format_rule(day) failed");
    rc = dvz_datetime_format_rule(datetime, DVZ_TIME_INTERVAL_MONTH, "%Y-%m");
    EXAMPLE_CHECK(rc == 0, "dvz_datetime_format_rule(month) failed");
    rc = dvz_datetime_format_rule(datetime, DVZ_TIME_INTERVAL_YEAR, "%Y");
    EXAMPLE_CHECK(rc == 0, "dvz_datetime_format_rule(year) failed");
    const DvzTimestamp may_1_utc = (DvzTimestamp)1714554000000000LL; /* 2024-05-01 09:00 UTC */
    ok = dvz_axis_set_datetime(x_axis, datetime);
    EXAMPLE_CHECK(ok, "dvz_axis_set_datetime() failed");
    ok = dvz_axis_set_datetime_range(
        x_axis, 0.0, 8.0, may_1_utc, may_1_utc + 8LL * 3600LL * 1000000LL);
    EXAMPLE_CHECK(ok, "dvz_axis_set_datetime_range() failed");

    ok = dvz_axis_set_label(x_axis, "UTC time");
    EXAMPLE_CHECK(ok, "dvz_axis_set_label() failed for X");
    ok = dvz_axis_set_label(y_axis, "signal");
    EXAMPLE_CHECK(ok, "dvz_axis_set_label() failed for Y");

    app = dvz_app(scene);
    EXAMPLE_CHECK(app != NULL, "dvz_app() failed (no GPU or display?)");

    DvzView* win = dvz_view_glfw(app, figure, WIDTH, HEIGHT, "datetime_axis");
    EXAMPLE_CHECK(win != NULL, "dvz_view_glfw() failed (GLFW unavailable?)");

    DvzController* panzoom_controller = dvz_panzoom(scene, NULL);
    EXAMPLE_CHECK(panzoom_controller != NULL, "dvz_panzoom() failed");
    DvzPanzoom* panzoom = dvz_controller_panzoom(panzoom_controller);
    EXAMPLE_CHECK(panzoom != NULL, "dvz_controller_panzoom() failed");
    rc = dvz_view_bind_controller(win, panel, panzoom_controller, DVZ_DIM_MASK_X);
    EXAMPLE_CHECK(rc == 0, "dvz_view_bind_controller() failed for X panzoom");

    int rc_capture = dvz_view_capture_from_env(win, "datetime_axis");
    EXAMPLE_CHECK(rc_capture == 0, "dvz_view_capture_from_env() failed");

    dvz_app_run(app, example_frame_count(argc, argv));

    rc_capture = dvz_view_capture_stop(win);
    EXAMPLE_CHECK(rc_capture == 0, "dvz_view_capture_stop() failed");
    ret = 0;

cleanup:
    if (app != NULL)
        dvz_app_destroy(app);
    if (scene != NULL)
        dvz_scene_destroy(scene);
    return ret;
}
