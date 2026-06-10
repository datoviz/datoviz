/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* panel_linked_axes - linked temporal panels with a spanning summary panel.
 *
 * Scenario: linked_panels_axes_panzoom
 * Style: showcase workflow, graphite_cyan, 1600x1200 capture target
 *
 * Build:  just example-c showcases/panel_linked_axes
 * Run:    ./build/examples/c/showcases/panel_linked_axes --live
 */



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <math.h>
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

DvzScenarioSpec dvz_showcase_linked_panel_axes_scenario(void);



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH             1600u
#define HEIGHT            1200u
#define PATH_COUNT        360u
#define EVENT_COUNT       96u
#define EVENT_ROWS        8u
#define POINT_COUNT       168u
#define PHASE_COUNT       360u
#define BAND_COUNT        2u
#define BAND_VERTEX_COUNT (6u * BAND_COUNT)
#define CURSOR_COUNT      BAND_COUNT

static const float TAU = 6.28318530718f;

#define LINKED_AXES_CHECK(condition, message)                                                    \
    do                                                                                            \
    {                                                                                             \
        if (!(condition))                                                                         \
        {                                                                                         \
            dvz_fprintf(stderr, "%s\n", message);                                                \
            return false;                                                                         \
        }                                                                                         \
    } while (0)



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Return the deterministic signal used across panels.
 *
 * @param t normalized time in [0, 1]
 * @return signal value
 */
static float _signal(float t)
{
    return 0.85f * sinf(TAU * (1.08f * t + 0.06f)) +
           0.34f * sinf(TAU * (3.20f * t + 0.18f)) +
           0.12f * cosf(TAU * (6.00f * t + 0.10f));
}



/**
 * Return the deterministic secondary signal used for summaries.
 *
 * @param t normalized time in [0, 1]
 * @return secondary signal value
 */
static float _lagged_signal(float t)
{
    return 0.76f * sinf(TAU * (1.08f * t - 0.09f)) +
           0.30f * sinf(TAU * (2.50f * t + 0.32f));
}



/**
 * Fill deterministic signal path data in data coordinates.
 *
 * @param positions output data-space path positions
 * @param colors output path colors
 * @param widths output path stroke widths
 * @param count sample count
 */
static void _fill_signal(vec3* positions, DvzColor* colors, float* widths, uint32_t count)
{
    ANN(positions);
    ANN(colors);
    ANN(widths);

    const float inv_count = count > 1 ? 1.0f / (float)(count - 1u) : 1.0f;
    for (uint32_t i = 0; i < count; i++)
    {
        const float t = (float)i * inv_count;
        positions[i][0] = 12.0f * t;
        positions[i][1] = _signal(t);
        positions[i][2] = 0.0f;

        colors[i] = dvz_color_rgba(72, (uint8_t)(188.0f + 44.0f * t), 242, 255);
        widths[i] = 3.2f;
    }
}



/**
 * Fill event raster segments in data coordinates.
 *
 * @param starts output segment starts
 * @param ends output segment ends
 * @param colors output segment colors
 * @param widths output segment widths
 * @param count event count
 */
static void _fill_events(
    vec3* starts, vec3* ends, DvzColor* colors, float* widths, uint32_t count)
{
    ANN(starts);
    ANN(ends);
    ANN(colors);
    ANN(widths);

    for (uint32_t i = 0; i < count; i++)
    {
        const uint32_t row = i % EVENT_ROWS;
        const uint32_t group = i / EVENT_ROWS;
        const float base = (float)group / (float)((count / EVENT_ROWS) - 1u);
        const float phase = (float)row / (float)(EVENT_ROWS - 1u);
        const float x = 12.0f * base + 0.18f * sinf(TAU * (0.23f * (float)i + 0.17f * phase));
        const float y0 = (float)row - 0.34f;
        const float y1 = (float)row + 0.34f;

        starts[i][0] = fminf(fmaxf(x, 0.0f), 12.0f);
        starts[i][1] = y0;
        starts[i][2] = 0.0f;
        ends[i][0] = starts[i][0];
        ends[i][1] = y1;
        ends[i][2] = 0.0f;

        colors[i] = dvz_color_rgba(100, (uint8_t)(170u + 9u * row), 220, 230);
        widths[i] = 2.2f + 0.8f * (float)(row % 3u);
    }
}



/**
 * Fill deterministic residual point data in data coordinates.
 *
 * @param positions output data-space point positions
 * @param colors output point colors
 * @param diameters output point diameters
 * @param count point count
 */
static void _fill_residuals(vec3* positions, DvzColor* colors, float* diameters, uint32_t count)
{
    ANN(positions);
    ANN(colors);
    ANN(diameters);

    const float inv_count = count > 1 ? 1.0f / (float)(count - 1u) : 1.0f;
    for (uint32_t i = 0; i < count; i++)
    {
        const float t = (float)i * inv_count;
        const float x = 12.0f * t;
        const float y = 0.55f * (_signal(t) - _lagged_signal(t)) +
                        0.12f * sinf(TAU * (9.0f * t + 0.20f));

        positions[i][0] = x;
        positions[i][1] = y;
        positions[i][2] = 0.0f;

        const float mag = fminf(fabsf(y), 1.0f);
        colors[i] = dvz_color_rgba(
            (uint8_t)(110.0f + 65.0f * mag), (uint8_t)(170.0f + 52.0f * (1.0f - mag)),
            216, 238);
        diameters[i] = 4.0f + 5.0f * mag;
    }
}



/**
 * Fill phase portrait data in the right-side summary panel.
 *
 * @param positions output data-space path positions
 * @param colors output path colors
 * @param widths output path stroke widths
 * @param count sample count
 */
static void _fill_phase(vec3* positions, DvzColor* colors, float* widths, uint32_t count)
{
    ANN(positions);
    ANN(colors);
    ANN(widths);

    const float inv_count = count > 1 ? 1.0f / (float)(count - 1u) : 1.0f;
    for (uint32_t i = 0; i < count; i++)
    {
        const float t = (float)i * inv_count;
        positions[i][0] = _signal(t);
        positions[i][1] = _lagged_signal(t);
        positions[i][2] = 0.0f;

        colors[i] = dvz_color_rgba((uint8_t)(64.0f + 64.0f * t), 214, 205, 245);
        widths[i] = 2.4f;
    }
}



/**
 * Fill synchronized X bands in one panel's data coordinates.
 *
 * @param ymin panel data-domain minimum
 * @param ymax panel data-domain maximum
 * @param positions output band triangle positions
 * @param colors output band colors
 */
static void _fill_bands(double ymin, double ymax, vec3* positions, DvzColor* colors)
{
    ANN(positions);
    ANN(colors);

    const float x0[BAND_COUNT] = {3.10f, 8.05f};
    const float x1[BAND_COUNT] = {3.76f, 8.72f};
    const DvzColor band_colors[BAND_COUNT] = {
        {72, 170, 205, 45},
        {128, 220, 185, 38},
    };

    for (uint32_t b = 0; b < BAND_COUNT; b++)
    {
        const uint32_t k = 6u * b;
        positions[k + 0][0] = x0[b];
        positions[k + 0][1] = (float)ymin;
        positions[k + 1][0] = x1[b];
        positions[k + 1][1] = (float)ymin;
        positions[k + 2][0] = x1[b];
        positions[k + 2][1] = (float)ymax;
        positions[k + 3][0] = x0[b];
        positions[k + 3][1] = (float)ymin;
        positions[k + 4][0] = x1[b];
        positions[k + 4][1] = (float)ymax;
        positions[k + 5][0] = x0[b];
        positions[k + 5][1] = (float)ymax;
        for (uint32_t j = 0; j < 6u; j++)
        {
            positions[k + j][2] = -0.02f;
            colors[k + j] = band_colors[b];
        }
    }
}



/**
 * Fill fixed-pixel-width cursor lines in one panel's data coordinates.
 *
 * @param ymin panel data-domain minimum
 * @param ymax panel data-domain maximum
 * @param starts output cursor segment starts
 * @param ends output cursor segment ends
 * @param colors output cursor colors
 * @param widths output cursor line widths
 */
static void _fill_cursor_lines(
    double ymin, double ymax, vec3* starts, vec3* ends, DvzColor* colors, float* widths)
{
    ANN(starts);
    ANN(ends);
    ANN(colors);
    ANN(widths);

    const float x[CURSOR_COUNT] = {3.43f, 8.38f};
    const DvzColor line_colors[CURSOR_COUNT] = {
        {100, 220, 245, 185},
        {150, 240, 205, 175},
    };

    for (uint32_t i = 0; i < CURSOR_COUNT; i++)
    {
        starts[i][0] = x[i];
        starts[i][1] = (float)ymin;
        starts[i][2] = 0.0f;
        ends[i][0] = x[i];
        ends[i][1] = (float)ymax;
        ends[i][2] = 0.0f;
        colors[i] = line_colors[i];
        widths[i] = 2.2f;
    }
}



/**
 * Configure retained X/Y axes on one panel.
 *
 * @param panel target panel
 * @param x_label X axis label
 * @param y_label Y axis label
 * @return true when all axis calls succeed
 */
static bool _add_axes(DvzPanel* panel, const char* x_label, const char* y_label)
{
    ANN(panel);
    ANN(y_label);

    DvzAxis* x_axis = dvz_panel_axis(panel, DVZ_DIM_X);
    DvzAxis* y_axis = dvz_panel_axis(panel, DVZ_DIM_Y);
    if (x_axis == NULL || y_axis == NULL)
        return false;

    DvzAxisTickPolicy ticks = dvz_axis_tick_policy();
    ticks.target_count = 6;
    ticks.min_pixel_spacing = 96.0f;
    ticks.minor_per_interval = 3;
    if (!dvz_axis_set_tick_policy(x_axis, &ticks))
        return false;
    if (!dvz_axis_set_tick_policy(y_axis, &ticks))
        return false;
    ExampleAxisStyleOptions style = example_graphite_cyan_axis_options();
    style.tick_size_px = 11.0f;
    style.label_size_px = 14.0f;
    style.tick_gap_px = 7.0f;
    style.x_label_gap_px = 32.0f;
    style.y_label_gap_px = 42.0f;
    style.minor_tick_alpha = 210;
    style.grid_alpha = 145;
    if (!example_graphite_cyan_apply_axis_style(x_axis, false, &style) ||
        !example_graphite_cyan_apply_axis_style(y_axis, true, &style))
        return false;
    if (!dvz_axis_set_grid(x_axis, true) || !dvz_axis_set_grid(y_axis, true))
        return false;
    if (x_label != NULL && !dvz_axis_set_label(x_axis, x_label))
        return false;
    return dvz_axis_set_label(y_axis, y_label);
}



/**
 * Add synchronized reference bands clipped to one panel.
 *
 * @param scene scene owning the visual
 * @param panel panel receiving the visual
 * @param ymin panel data-domain minimum
 * @param ymax panel data-domain maximum
 * @return true when the bands were added
 */
static bool _add_bands(DvzScene* scene, DvzPanel* panel, double ymin, double ymax)
{
    ANN(scene);
    ANN(panel);

    vec3 data_positions[BAND_VERTEX_COUNT] = {{0}};
    vec3 visual_positions[BAND_VERTEX_COUNT] = {{0}};
    DvzColor colors[BAND_VERTEX_COUNT] = {{0}};
    _fill_bands(ymin, ymax, data_positions, colors);

    int rc = dvz_panel_data_to_visual_positions(
        panel, (const float*)data_positions, (float*)visual_positions, BAND_VERTEX_COUNT);
    if (rc != 0)
        return false;

    DvzVisual* visual = dvz_primitive(scene, DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0);
    if (visual == NULL)
        return false;
    if (dvz_visual_set_data(visual, "position", visual_positions, BAND_VERTEX_COUNT) != 0)
        return false;
    if (dvz_visual_set_data(visual, "color", colors, BAND_VERTEX_COUNT) != 0)
        return false;
    if (dvz_visual_set_alpha_mode(visual, DVZ_ALPHA_BLENDED) != 0)
        return false;
    if (dvz_visual_set_depth_test(visual, false) != 0)
        return false;
    return dvz_panel_add_visual(panel, visual, NULL) == 0;
}



/**
 * Add synchronized fixed-width cursor lines clipped to one panel.
 *
 * @param scene scene owning the visual
 * @param panel panel receiving the visual
 * @param ymin panel data-domain minimum
 * @param ymax panel data-domain maximum
 * @return true when the cursor lines were added
 */
static bool _add_cursor_lines(DvzScene* scene, DvzPanel* panel, double ymin, double ymax)
{
    ANN(scene);
    ANN(panel);

    vec3 starts[CURSOR_COUNT] = {{0}};
    vec3 ends[CURSOR_COUNT] = {{0}};
    vec3 visual_starts[CURSOR_COUNT] = {{0}};
    vec3 visual_ends[CURSOR_COUNT] = {{0}};
    DvzColor colors[CURSOR_COUNT] = {{0}};
    float widths[CURSOR_COUNT] = {0};
    _fill_cursor_lines(ymin, ymax, starts, ends, colors, widths);

    int rc = dvz_panel_data_to_visual_positions(
        panel, (const float*)starts, (float*)visual_starts, CURSOR_COUNT);
    if (rc != 0)
        return false;
    rc = dvz_panel_data_to_visual_positions(
        panel, (const float*)ends, (float*)visual_ends, CURSOR_COUNT);
    if (rc != 0)
        return false;

    DvzVisual* visual = dvz_segment(scene, 0);
    if (visual == NULL)
        return false;
    DvzVisualDataUpdate updates[] = {
        {.attr_name = "position_start", .data = visual_starts, .item_count = CURSOR_COUNT},
        {.attr_name = "position_end", .data = visual_ends, .item_count = CURSOR_COUNT},
        {.attr_name = "color", .data = colors, .item_count = CURSOR_COUNT},
        {.attr_name = "stroke_width", .data = widths, .item_count = CURSOR_COUNT},
    };
    if (dvz_visual_set_data_many(visual, updates, 4) != 0)
        return false;
    if (dvz_segment_set_caps(visual, DVZ_SEGMENT_CAP_SQUARE, DVZ_SEGMENT_CAP_SQUARE) != 0)
        return false;
    if (dvz_visual_set_depth_test(visual, false) != 0)
        return false;
    return dvz_panel_add_visual(panel, visual, NULL) == 0;
}



/**
 * Add a stroked path visual to one panel.
 *
 * @param scene scene owning the visual
 * @param panel panel receiving the visual
 * @param positions data-space path positions
 * @param colors path colors
 * @param widths path stroke widths
 * @param count sample count
 * @return true when the visual was added
 */
static bool _add_path(
    DvzScene* scene, DvzPanel* panel, vec3* positions, DvzColor* colors, float* widths,
    uint32_t count)
{
    ANN(scene);
    ANN(panel);
    ANN(positions);
    ANN(colors);
    ANN(widths);

    vec3 visual_positions[PHASE_COUNT] = {{0}};
    if (count > PHASE_COUNT)
        return false;
    int rc = dvz_panel_data_to_visual_positions(
        panel, (const float*)positions, (float*)visual_positions, count);
    if (rc != 0)
        return false;

    DvzVisual* visual = dvz_path(scene, 0);
    if (visual == NULL)
        return false;

    DvzVisualDataUpdate updates[] = {
        {.attr_name = "position", .data = visual_positions, .item_count = count},
        {.attr_name = "color", .data = colors, .item_count = count},
        {.attr_name = "stroke_width", .data = widths, .item_count = count},
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



/**
 * Add the top signal path panel.
 *
 * @param scene scene owning the visual
 * @param panel panel receiving the visual
 * @return true when the visual was added
 */
static bool _add_signal_panel(DvzScene* scene, DvzPanel* panel)
{
    ANN(scene);
    ANN(panel);

    vec3 positions[PATH_COUNT] = {{0}};
    DvzColor colors[PATH_COUNT] = {{0}};
    float widths[PATH_COUNT] = {0};
    _fill_signal(positions, colors, widths, PATH_COUNT);
    return _add_path(scene, panel, positions, colors, widths, PATH_COUNT);
}



/**
 * Add the middle event raster panel.
 *
 * @param scene scene owning the visual
 * @param panel panel receiving the visual
 * @return true when the visual was added
 */
static bool _add_event_panel(DvzScene* scene, DvzPanel* panel)
{
    ANN(scene);
    ANN(panel);

    vec3 starts[EVENT_COUNT] = {{0}};
    vec3 ends[EVENT_COUNT] = {{0}};
    vec3 visual_starts[EVENT_COUNT] = {{0}};
    vec3 visual_ends[EVENT_COUNT] = {{0}};
    DvzColor colors[EVENT_COUNT] = {{0}};
    float widths[EVENT_COUNT] = {0};
    _fill_events(starts, ends, colors, widths, EVENT_COUNT);

    int rc = dvz_panel_data_to_visual_positions(
        panel, (const float*)starts, (float*)visual_starts, EVENT_COUNT);
    if (rc != 0)
        return false;
    rc = dvz_panel_data_to_visual_positions(
        panel, (const float*)ends, (float*)visual_ends, EVENT_COUNT);
    if (rc != 0)
        return false;

    DvzVisual* visual = dvz_segment(scene, 0);
    if (visual == NULL)
        return false;

    DvzVisualDataUpdate updates[] = {
        {.attr_name = "position_start", .data = visual_starts, .item_count = EVENT_COUNT},
        {.attr_name = "position_end", .data = visual_ends, .item_count = EVENT_COUNT},
        {.attr_name = "color", .data = colors, .item_count = EVENT_COUNT},
        {.attr_name = "stroke_width", .data = widths, .item_count = EVENT_COUNT},
    };
    if (dvz_visual_set_data_many(visual, updates, 4) != 0)
        return false;
    if (dvz_segment_set_caps(visual, DVZ_SEGMENT_CAP_SQUARE, DVZ_SEGMENT_CAP_SQUARE) != 0)
        return false;
    if (dvz_visual_set_depth_test(visual, false) != 0)
        return false;
    return dvz_panel_add_visual(panel, visual, NULL) == 0;
}



/**
 * Add the bottom residual point panel.
 *
 * @param scene scene owning the visual
 * @param panel panel receiving the visual
 * @return true when the visual was added
 */
static bool _add_residual_panel(DvzScene* scene, DvzPanel* panel)
{
    ANN(scene);
    ANN(panel);

    vec3 data_positions[POINT_COUNT] = {{0}};
    vec3 visual_positions[POINT_COUNT] = {{0}};
    DvzColor colors[POINT_COUNT] = {{0}};
    float diameters[POINT_COUNT] = {0};
    _fill_residuals(data_positions, colors, diameters, POINT_COUNT);

    int rc = dvz_panel_data_to_visual_positions(
        panel, (const float*)data_positions, (float*)visual_positions, POINT_COUNT);
    if (rc != 0)
        return false;

    DvzVisual* visual = dvz_point(scene, 0);
    if (visual == NULL)
        return false;

    DvzVisualDataUpdate updates[] = {
        {.attr_name = "position", .data = visual_positions, .item_count = POINT_COUNT},
        {.attr_name = "color", .data = colors, .item_count = POINT_COUNT},
        {.attr_name = "diameter", .data = diameters, .item_count = POINT_COUNT},
    };
    if (dvz_visual_set_data_many(visual, updates, 3) != 0)
        return false;

    DvzPointStyleDesc style = dvz_point_style_desc();
    style.aspect = DVZ_SHAPE_ASPECT_FILLED;
    if (dvz_point_set_style(visual, &style) != 0)
        return false;
    if (dvz_visual_set_depth_test(visual, false) != 0)
        return false;
    return dvz_panel_add_visual(panel, visual, NULL) == 0;
}



/**
 * Add the right-side phase summary panel.
 *
 * @param scene scene owning the visual
 * @param panel panel receiving the visual
 * @return true when the visual was added
 */
static bool _add_summary_panel(DvzScene* scene, DvzPanel* panel)
{
    ANN(scene);
    ANN(panel);

    vec3 positions[PHASE_COUNT] = {{0}};
    DvzColor colors[PHASE_COUNT] = {{0}};
    float widths[PHASE_COUNT] = {0};
    _fill_phase(positions, colors, widths, PHASE_COUNT);
    return _add_path(scene, panel, positions, colors, widths, PHASE_COUNT);
}



/**
 * Bind shared X panzoom and independent Y/summary panzooms.
 *
 * @param ctx scenario context
 * @param left left-column panels
 * @param left_count number of left-column panels
 * @param summary right summary panel
 * @return true when controllers and input routing are ready
 */
static bool _bind_linked_panzooms(
    DvzScenarioContext* ctx, DvzPanel** left, uint32_t left_count, DvzPanel* summary)
{
    ANN(ctx);
    ANN(left);
    ANN(summary);

    DvzController* shared_x = dvz_panzoom(ctx->scene, NULL);
    DvzController* summary_xy = dvz_panzoom(ctx->scene, NULL);
    if (shared_x == NULL || summary_xy == NULL)
        return false;

    for (uint32_t i = 0; i < left_count; i++)
    {
        DvzController* y = dvz_panzoom(ctx->scene, NULL);
        if (left[i] == NULL || y == NULL)
            return false;
        if (dvz_scenario_bind_controller(ctx, left[i], shared_x, DVZ_DIM_MASK_X) != 0)
            return false;
        if (dvz_scenario_bind_controller(ctx, left[i], y, DVZ_DIM_MASK_Y) != 0)
            return false;
    }
    return dvz_scenario_bind_controller(ctx, summary, summary_xy, DVZ_DIM_MASK_XY) == 0;
}



/**
 * Set one panel domain with checked return values.
 *
 * @param panel target panel
 * @param x0 X minimum
 * @param x1 X maximum
 * @param y0 Y minimum
 * @param y1 Y maximum
 * @return true when both domains were set
 */
static bool _set_domains(DvzPanel* panel, double x0, double x1, double y0, double y1)
{
    ANN(panel);

    int rc = dvz_panel_set_domain(panel, DVZ_DIM_X, x0, x1);
    if (rc != 0)
        return false;
    rc = dvz_panel_set_domain(panel, DVZ_DIM_Y, y0, y1);
    return rc == 0;
}



/**
 * Add a subtle fixed border around one panel.
 *
 * @param panel target panel
 * @return true when the border was configured
 */
static bool _set_panel_border(DvzPanel* panel)
{
    ANN(panel);

    DvzColor color = example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_GRID);
    color.a = 220u;
    DvzPanelBorderDesc border = dvz_panel_border_desc();
    border.color = color;
    border.width_px = 2.25f;
    border.inset_px = 1.125f;
    return dvz_panel_set_border(panel, &border);
}



/**
 * Configure one panel's background and layout reserve.
 *
 * @param panel target panel
 * @param bottom bottom layout reserve
 * @return true when layout was configured
 */
static bool _configure_panel(DvzPanel* panel, float bottom)
{
    ANN(panel);

    example_graphite_cyan_set_panel_background(panel);
    bool ok = dvz_panel_set_layout_reserve(
        panel, &(DvzPanelLayoutReserve){.left = 0.20f, .right = 0.06f, .bottom = bottom,
                                        .top = 0.07f});
    return ok && _set_panel_border(panel);
}



/**
 * Initialize the linked-panel axes scenario.
 *
 * @param ctx scenario context
 * @param out_user scenario state output
 * @return whether setup succeeded
 */
static bool _scenario_init(DvzScenarioContext* ctx, void** out_user)
{
    ANN(ctx);
    ANN(ctx->scene);
    if (out_user != NULL)
        *out_user = NULL;

    ctx->figure = dvz_figure(ctx->scene, ctx->width, ctx->height, 0);
    LINKED_AXES_CHECK(ctx->figure != NULL, "dvz_figure() failed");
    DvzGrid* grid = dvz_figure_grid(ctx->figure, 3, 2);
    LINKED_AXES_CHECK(grid != NULL, "dvz_figure_grid() failed");
    bool ok = dvz_grid_set_margins(
        grid, &(DvzPanelReserve){.left_px = 36.0f, .right_px = 30.0f, .top_px = 24.0f,
                                 .bottom_px = 30.0f});
    LINKED_AXES_CHECK(ok, "dvz_grid_set_margins() failed");
    ok = dvz_grid_set_gutter(grid, 28.0f, 26.0f);
    LINKED_AXES_CHECK(ok, "dvz_grid_set_gutter() failed");

    DvzPanel* signal = dvz_grid_panel(grid, 0, 0);
    DvzPanel* events = dvz_grid_panel(grid, 1, 0);
    DvzPanel* residuals = dvz_grid_panel(grid, 2, 0);
    DvzPanel* summary = dvz_grid_panel_span(grid, 0, 1, 3, 1);
    LINKED_AXES_CHECK(
        signal != NULL && events != NULL && residuals != NULL && summary != NULL,
        "dvz_grid_panel() failed");

    ok = _configure_panel(signal, 0.20f);
    LINKED_AXES_CHECK(ok, "_configure_panel(signal) failed");
    ok = _configure_panel(events, 0.20f);
    LINKED_AXES_CHECK(ok, "_configure_panel(events) failed");
    ok = _configure_panel(residuals, 0.36f);
    LINKED_AXES_CHECK(ok, "_configure_panel(residuals) failed");
    ok = dvz_panel_set_layout_reserve(
        summary, &(DvzPanelLayoutReserve){.left = 0.20f, .right = 0.08f, .bottom = 0.13f,
                                          .top = 0.06f});
    LINKED_AXES_CHECK(ok, "dvz_panel_set_layout_reserve(summary) failed");
    example_graphite_cyan_set_panel_background(summary);
    ok = _set_panel_border(summary);
    LINKED_AXES_CHECK(ok, "_set_panel_border(summary) failed");

    ok = _set_domains(signal, 0.0, 12.0, -1.6, 1.6);
    LINKED_AXES_CHECK(ok, "_set_domains(signal) failed");
    ok = _set_domains(events, 0.0, 12.0, -0.8, 7.8);
    LINKED_AXES_CHECK(ok, "_set_domains(events) failed");
    ok = _set_domains(residuals, 0.0, 12.0, -1.0, 1.0);
    LINKED_AXES_CHECK(ok, "_set_domains(residuals) failed");
    ok = _set_domains(summary, -1.45, 1.45, -1.45, 1.45);
    LINKED_AXES_CHECK(ok, "_set_domains(summary) failed");

    ok = _add_bands(ctx->scene, signal, -1.6, 1.6);
    LINKED_AXES_CHECK(ok, "_add_bands(signal) failed");
    ok = _add_bands(ctx->scene, events, -0.8, 7.8);
    LINKED_AXES_CHECK(ok, "_add_bands(events) failed");
    ok = _add_bands(ctx->scene, residuals, -1.0, 1.0);
    LINKED_AXES_CHECK(ok, "_add_bands(residuals) failed");
    ok = _add_signal_panel(ctx->scene, signal);
    LINKED_AXES_CHECK(ok, "_add_signal_panel() failed");
    ok = _add_event_panel(ctx->scene, events);
    LINKED_AXES_CHECK(ok, "_add_event_panel() failed");
    ok = _add_residual_panel(ctx->scene, residuals);
    LINKED_AXES_CHECK(ok, "_add_residual_panel() failed");
    ok = _add_summary_panel(ctx->scene, summary);
    LINKED_AXES_CHECK(ok, "_add_summary_panel() failed");
    ok = _add_cursor_lines(ctx->scene, signal, -1.6, 1.6);
    LINKED_AXES_CHECK(ok, "_add_cursor_lines(signal) failed");
    ok = _add_cursor_lines(ctx->scene, events, -0.8, 7.8);
    LINKED_AXES_CHECK(ok, "_add_cursor_lines(events) failed");
    ok = _add_cursor_lines(ctx->scene, residuals, -1.0, 1.0);
    LINKED_AXES_CHECK(ok, "_add_cursor_lines(residuals) failed");

    ok = _add_axes(signal, NULL, "signal");
    LINKED_AXES_CHECK(ok, "_add_axes(signal) failed");
    ok = _add_axes(events, NULL, "events");
    LINKED_AXES_CHECK(ok, "_add_axes(events) failed");
    ok = _add_axes(residuals, "time (s)", "residual");
    LINKED_AXES_CHECK(ok, "_add_axes(residuals) failed");
    ok = _add_axes(summary, "signal", "lagged");
    LINKED_AXES_CHECK(ok, "_add_axes(summary) failed");

    DvzPanel* left[] = {signal, events, residuals};
    ok = _bind_linked_panzooms(ctx, left, 3, summary);
    LINKED_AXES_CHECK(ok, "_bind_linked_panzooms() failed");

    return true;
}



/**
 * Return the linked-panel axes scenario specification.
 *
 * @return scenario specification
 */
DvzScenarioSpec dvz_showcase_linked_panel_axes_scenario(void)
{
    return (DvzScenarioSpec){
        .id = "linked_panels_axes_panzoom",
        .title = "panel_linked_axes",
        .width = WIDTH,
        .height = HEIGHT,
        .fps = 60.0,
        .requirements = DVZ_SCENARIO_REQ_POINT_VISUAL | DVZ_SCENARIO_REQ_TEXT_VISUAL |
                        DVZ_SCENARIO_REQ_CONTROLLER | DVZ_SCENARIO_REQ_PANZOOM,
        .init = _scenario_init,
    };
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Run the linked-panel axes and shared-X panzoom feature proof.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @return process exit code
 */
#ifndef DVZ_EXAMPLE_NO_MAIN
int main(int argc, char** argv)
{
    DvzScenarioSpec spec = dvz_showcase_linked_panel_axes_scenario();
    return dvz_scenario_run_native_cli(&spec, argc, argv) == 0 ? 0 : 1;
}
#endif

#undef LINKED_AXES_CHECK
