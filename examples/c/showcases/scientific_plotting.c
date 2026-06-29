/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* scientific_plotting - histogram, guide annotations, stacked traces, and error bands.
 *
 * Scenario: scientific_plotting_workflow
 * Style: showcase workflow, graphite_cyan, 1600x1200 capture target
 *
 * Build:  just example-c showcases/scientific_plotting
 * Run:    ./build/examples/c/showcases/scientific_plotting --live
 * Smoke:  ./build/examples/c/showcases/scientific_plotting --png
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



DvzScenarioSpec dvz_showcase_scientific_plotting_scenario(void);



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH                  1600u
#define HEIGHT                 1200u
#define CORR_BINS              101u
#define MEAN_COUNT             320u
#define TRACE_COUNT            32u
#define TRACE_SAMPLES          320u
#define TRACE_VERTEX_COUNT     (TRACE_COUNT * TRACE_SAMPLES)

static const float TAU = 6.28318530718f;



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Linearly blend two 8-bit channels.
 *
 * @param a first channel
 * @param b second channel
 * @param t blend factor in [0, 1]
 * @return blended channel
 */
static uint8_t _lerp_u8(uint8_t a, uint8_t b, float t)
{
    return (uint8_t)((1.0f - t) * (float)a + t * (float)b);
}



/**
 * Blend two palette colors.
 *
 * @param a first color
 * @param b second color
 * @param t blend factor in [0, 1]
 * @param alpha output alpha
 * @return blended RGBA color
 */
static DvzColor _lerp_color(DvzColor a, DvzColor b, float t, uint8_t alpha)
{
    return dvz_color_rgba(_lerp_u8(a.r, b.r, t), _lerp_u8(a.g, b.g, t), _lerp_u8(a.b, b.b, t),
                          alpha);
}



/**
 * Return the stacked-trace color for one channel.
 *
 * @param channel channel index
 * @return trace color
 */
static DvzColor _trace_color(uint32_t channel)
{
    const float u = TRACE_COUNT > 1 ? (float)channel / (float)(TRACE_COUNT - 1u) : 0.0f;
    DvzColor cyan = example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ACCENT_PRIMARY);
    DvzColor mint = example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ACCENT_SECONDARY);
    DvzColor amber = example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_WARNING);
    DvzColor rose = example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ERROR);
    if (u < 0.38f)
        return _lerp_color(cyan, mint, u / 0.38f, 235);
    if (u < 0.74f)
        return _lerp_color(mint, amber, (u - 0.38f) / 0.36f, 235);
    return _lerp_color(amber, rose, (u - 0.74f) / 0.26f, 235);
}



/**
 * Return a deterministic autocorrelogram value.
 *
 * @param lag_ms lag in milliseconds
 * @return count estimate
 */
static double _autocorr_value(double lag_ms)
{
    const double baseline = 38.0;
    const double peak = 88.0 * exp(-(lag_ms * lag_ms) / (2.0 * 7.4 * 7.4));
    const double theta = 10.0 * cos(0.31 * lag_ms) * exp(-fabs(lag_ms) / 34.0);
    const double refractory = 92.0 * exp(-(lag_ms * lag_ms) / (2.0 * 1.8 * 1.8));
    double value = baseline + peak + theta - refractory;
    return value < 1.0 ? 1.0 : value;
}



/**
 * Fill explicit interval bars for the spike-train autocorrelogram.
 *
 * @param starts output bin starts
 * @param ends output bin ends
 * @param values output bin values
 */
static void _fill_autocorrelogram(double starts[CORR_BINS], double ends[CORR_BINS],
                                  double values[CORR_BINS])
{
    const double min_lag = -50.0;
    const double bin_width = 100.0 / (double)CORR_BINS;
    for (uint32_t i = 0; i < CORR_BINS; i++)
    {
        starts[i] = min_lag + (double)i * bin_width;
        ends[i] = starts[i] + bin_width;
        values[i] = _autocorr_value(0.5 * (starts[i] + ends[i]));
    }
}



/**
 * Add a visual to a panel in data coordinates.
 *
 * @param panel target panel
 * @param visual visual
 * @param z_layer draw layer
 * @return true when the visual was attached
 */
static bool _add_data_visual(DvzPanel* panel, DvzVisual* visual, int32_t z_layer)
{
    DvzVisualAttachDesc attach = dvz_visual_attach_desc();
    attach.coord_space = DVZ_COORD_DATA;
    attach.z_layer = z_layer;
    return dvz_panel_add_visual(panel, visual, &attach) == 0;
}



/**
 * Add retained axes with compact showcase styling.
 *
 * @param panel target panel
 * @param x_label optional X label
 * @param y_label optional Y label
 * @param show_y_axis whether to show the Y axis
 * @param x_label_gap_px X axis title gap in pixels
 * @return true when the axes were configured
 */
static bool _add_axes(
    DvzPanel* panel, const char* x_label, const char* y_label, bool show_y_axis,
    float x_label_gap_px)
{
    DvzAxis* x_axis = dvz_panel_axis(panel, DVZ_DIM_X);
    DvzAxis* y_axis = dvz_panel_axis(panel, DVZ_DIM_Y);
    if (x_axis == NULL || y_axis == NULL)
        return false;

    DvzAxisTickPolicy ticks = dvz_axis_tick_policy();
    ticks.target_count = 6;
    ticks.min_pixel_spacing = 92.0f;
    ticks.minor_per_interval = 3;
    if (!dvz_axis_set_tick_policy(x_axis, &ticks) || !dvz_axis_set_tick_policy(y_axis, &ticks))
        return false;

    ExampleAxisStyleOptions style = example_graphite_cyan_axis_options();
    style.tick_size_px = 10.0f;
    style.label_size_px = 13.0f;
    style.tick_gap_px = 6.0f;
    style.x_label_gap_px = x_label_gap_px;
    style.y_label_gap_px = 18.0f;
    style.grid_alpha = 105;
    if (!example_graphite_cyan_apply_axis_style(x_axis, false, &style))
        return false;
    if (!example_graphite_cyan_apply_axis_style(y_axis, true, &style))
        return false;
    if (!dvz_axis_set_grid(x_axis, true) || !dvz_axis_set_grid(y_axis, show_y_axis))
        return false;
    if (!dvz_axis_set_visible(y_axis, show_y_axis))
        return false;
    if (x_label != NULL && !dvz_axis_set_label(x_axis, x_label))
        return false;
    if (show_y_axis && y_label != NULL && !dvz_axis_set_label(y_axis, y_label))
        return false;
    return true;
}



/**
 * Configure one showcase panel.
 *
 * @param panel target panel
 * @return true when the panel was configured
 */
static bool _configure_panel(DvzPanel* panel)
{
    example_graphite_cyan_set_panel_background(panel);

    DvzColor color = example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_GRID);
    color.a = 220u;
    DvzPanelBorderDesc border = dvz_panel_border_desc();
    border.color = color;
    border.width_px = 2.0f;
    border.inset_px = 1.0f;
    return dvz_panel_set_border(panel, &border);
}



/**
 * Set a panel data domain.
 *
 * @param panel target panel
 * @param x0 X minimum
 * @param x1 X maximum
 * @param y0 Y minimum
 * @param y1 Y maximum
 * @return true when both domains were set
 */
static bool _set_domain(DvzPanel* panel, double x0, double x1, double y0, double y1)
{
    return dvz_panel_set_domain(panel, DVZ_DIM_X, x0, x1) == 0 &&
           dvz_panel_set_domain(panel, DVZ_DIM_Y, y0, y1) == 0;
}


/**
 * Add the autocorrelogram panel using bars and guide annotations.
 *
 * @param scene scene owning visuals
 * @param panel target panel
 * @return true when the panel was populated
 */
static bool _add_autocorrelogram(DvzScene* scene, DvzPanel* panel)
{
    (void)scene;
    double starts[CORR_BINS] = {0};
    double ends[CORR_BINS] = {0};
    double values[CORR_BINS] = {0};
    _fill_autocorrelogram(starts, ends, values);

    DvzBarsDesc bars_desc = dvz_bars_desc();
    bars_desc.fill_color = dvz_color_rgba(76, 201, 240, 185);
    bars_desc.outline_color = dvz_color_rgba(76, 201, 240, 75);
    bars_desc.outline_width_px = 0.8f;
    bars_desc.gap_fraction = 0.08f;
    DvzBars* bars = dvz_bars(panel, &bars_desc);
    if (bars == NULL || dvz_bars_set_intervals(bars, CORR_BINS, starts, ends, values) != 0)
        return false;

    DvzGuideSpanDesc span_desc = dvz_guide_span_desc();
    span_desc.fill_color = dvz_color_rgba(239, 71, 111, 42);
    span_desc.outline_color = dvz_color_rgba(239, 71, 111, 165);
    span_desc.outline_width_px = 1.5f;
    if (dvz_vspan(panel, -2.0, 2.0, &span_desc) == NULL)
        return false;

    DvzGuideLineDesc baseline = dvz_guide_line_desc();
    baseline.color = dvz_color_rgba(128, 255, 219, 220);
    baseline.stroke_width_px = 2.25f;
    if (dvz_hline(panel, 38.0, &baseline) == NULL)
        return false;

    DvzGuideLineDesc zero = dvz_guide_line_desc();
    zero.color = dvz_color_rgba(255, 183, 3, 220);
    zero.stroke_width_px = 2.0f;
    if (dvz_vline(panel, 0.0, &zero) == NULL)
        return false;

    DvzColor text = example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_TEXT);
    text.a = 235u;
    DvzColor baseline_text = example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ACCENT_SECONDARY);
    baseline_text.a = 235u;

    DvzLabelDesc label = dvz_label_desc();
    label.style = example_graphite_cyan_text_style(EXAMPLE_STYLE_TEXT_PANEL_LABEL);
    label.style.renderer = DVZ_TEXT_RENDERER_MSDF_ATLAS;
    label.placement.mode = DVZ_TEXT_PLACEMENT_DATA;
    label.placement.text_anchor[0] = 0.5f;
    label.placement.text_anchor[1] = 0.5f;
    label.placement.has_text_anchor = true;
    label.placement.depth_test = false;

    label.text = "bi-side refractory";
    label.style.color[0] = text.r;
    label.style.color[1] = text.g;
    label.style.color[2] = text.b;
    label.style.color[3] = text.a;
    label.placement.position[0] = 0.0f;
    label.placement.position[1] = 118.0f;
    label.placement.position[2] = 0.0f;
    label.placement.offset[0] = 0.0f;
    label.placement.offset[1] = 0.0f;
    if (dvz_annotation_label(panel, &label) == NULL)
        return false;

    label.text = "baseline";
    label.style.color[0] = baseline_text.r;
    label.style.color[1] = baseline_text.g;
    label.style.color[2] = baseline_text.b;
    label.style.color[3] = baseline_text.a;
    label.placement.position[0] = -30.0f;
    label.placement.position[1] = 38.0f;
    label.placement.position[2] = 0.0f;
    label.placement.offset[0] = 0.0f;
    label.placement.offset[1] = -14.0f;
    return dvz_annotation_label(panel, &label) != NULL;
}



/**
 * Fill mean path and uncertainty bounds.
 *
 * @param x output X coordinates
 * @param lower output lower bounds
 * @param upper output upper bounds
 * @param center output center line values
 */
static void _fill_mean_error(
    double x[MEAN_COUNT], double lower[MEAN_COUNT], double upper[MEAN_COUNT],
    double center[MEAN_COUNT])
{
    for (uint32_t i = 0; i < MEAN_COUNT; i++)
    {
        const float t = (float)i / (float)(MEAN_COUNT - 1u);
        const float y = 1.1f + 0.42f * sinf(TAU * (0.82f * t + 0.08f)) +
                        0.18f * sinf(TAU * (2.4f * t + 0.42f));
        const float e = 0.14f + 0.06f * sinf(TAU * (1.7f * t + 0.20f));
        x[i] = 10.0 * (double)t;
        center[i] = (double)y;
        lower[i] = (double)(y - e);
        upper[i] = (double)(y + e);
    }
}



/**
 * Add the mean path and translucent error band panel.
 *
 * @param scene scene owning visuals
 * @param panel target panel
 * @return true when the panel was populated
 */
static bool _add_mean_error(DvzScene* scene, DvzPanel* panel)
{
    (void)scene;
    double x[MEAN_COUNT] = {0};
    double lower[MEAN_COUNT] = {0};
    double upper[MEAN_COUNT] = {0};
    double center[MEAN_COUNT] = {0};
    _fill_mean_error(x, lower, upper, center);

    DvzBandDesc desc = dvz_band_desc();
    desc.fill_color = dvz_color_rgba(128, 255, 219, 58);
    desc.line_color = dvz_color_rgba(76, 201, 240, 255);
    desc.line_width_px = 5.5f;
    DvzBand* band = dvz_band(panel, &desc);
    return band != NULL && dvz_band_set_bounds(band, MEAN_COUNT, x, lower, upper) == 0 &&
           dvz_band_set_center(band, MEAN_COUNT, x, center) == 0;
}



/**
 * Fill high-density stacked traces.
 *
 * @param positions output positions
 * @param colors output colors
 * @param widths output widths
 * @param subpaths output subpath lengths
 */
static void _fill_stacked_traces(vec3 positions[TRACE_VERTEX_COUNT],
                                 DvzColor colors[TRACE_VERTEX_COUNT],
                                 float widths[TRACE_VERTEX_COUNT],
                                 uint32_t subpaths[TRACE_COUNT])
{
    for (uint32_t ch = 0; ch < TRACE_COUNT; ch++)
    {
        const float row = (float)(TRACE_COUNT - 1u - ch);
        const float phase = (float)ch / (float)TRACE_COUNT;
        DvzColor color = _trace_color(ch);
        subpaths[ch] = TRACE_SAMPLES;
        for (uint32_t i = 0; i < TRACE_SAMPLES; i++)
        {
            const float t = (float)i / (float)(TRACE_SAMPLES - 1u);
            const float y = row + 0.19f * sinf(TAU * ((2.1f + 0.035f * (float)ch) * t + phase)) +
                            0.055f * sinf(TAU * (27.0f * t + 0.11f * (float)ch)) +
                            0.035f * cosf(TAU * (53.0f * t + 0.07f * (float)ch));
            const uint32_t k = ch * TRACE_SAMPLES + i;
            positions[k][0] = 10.0f * t;
            positions[k][1] = y;
            positions[k][2] = 0.0f;
            colors[k] = color;
            widths[k] = ch == 0u || ch == TRACE_COUNT - 1u ? 1.9f : 1.55f;
        }
    }
}



/**
 * Add the stacked 32-trace panel.
 *
 * @param scene scene owning visuals
 * @param panel target panel
 * @return true when the panel was populated
 */
static bool _add_stacked_traces(DvzScene* scene, DvzPanel* panel)
{
    vec3 positions[TRACE_VERTEX_COUNT] = {{0}};
    DvzColor colors[TRACE_VERTEX_COUNT] = {{0}};
    float widths[TRACE_VERTEX_COUNT] = {0};
    uint32_t subpaths[TRACE_COUNT] = {0};
    _fill_stacked_traces(positions, colors, widths, subpaths);

    DvzGuideLineDesc cursor = dvz_guide_line_desc();
    cursor.color = dvz_color_rgba(76, 201, 240, 200);
    cursor.stroke_width_px = 1.75f;
    cursor.label = "probe";
    if (dvz_vline(panel, 5.0, &cursor) == NULL)
        return false;

    DvzVisual* traces = dvz_path(scene, 0);
    if (traces == NULL)
        return false;
    DvzVisualDataUpdate updates[] = {
        {.attr_name = "position", .data = positions, .item_count = TRACE_VERTEX_COUNT},
        {.attr_name = "color", .data = colors, .item_count = TRACE_VERTEX_COUNT},
        {.attr_name = "stroke_width_px", .data = widths, .item_count = TRACE_VERTEX_COUNT},
    };
    if (dvz_visual_set_data_many(traces, updates, 3) != 0)
        return false;
    if (dvz_path_set_subpaths(traces, TRACE_COUNT, subpaths) != 0)
        return false;
    if (dvz_path_set_caps(traces, DVZ_SEGMENT_CAP_ROUND, DVZ_SEGMENT_CAP_ROUND) != 0)
        return false;
    if (dvz_path_set_join(traces, DVZ_PATH_JOIN_ROUND, 4.0f) != 0)
        return false;
    if (dvz_visual_set_depth_test(traces, false) != 0)
        return false;
    return _add_data_visual(panel, traces, 1);
}



/*************************************************************************************************/
/*  Scenario callbacks                                                                           */
/*************************************************************************************************/

/**
 * Initialize the scientific plotting showcase.
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

    DvzGrid* grid = dvz_figure_grid(ctx->figure, 2, 2);
    if (grid == NULL)
        return false;
    if (!dvz_grid_set_margins(
            grid, &(DvzPanelReserve){.left_px = 24.0f, .right_px = 24.0f, .top_px = 18.0f,
                                     .bottom_px = 24.0f}))
        return false;
    if (!dvz_grid_set_gutter(grid, 24.0f, 34.0f))
        return false;

    DvzPanel* correlogram = dvz_grid_panel(grid, 0, 0);
    DvzPanel* mean_error = dvz_grid_panel(grid, 0, 1);
    DvzPanel* stacked = dvz_grid_panel_span(grid, 1, 0, 1, 2);
    if (correlogram == NULL || mean_error == NULL || stacked == NULL)
        return false;

    if (!_configure_panel(correlogram) || !_configure_panel(mean_error) || !_configure_panel(stacked))
        return false;

    if (!_set_domain(correlogram, -50.0, 50.0, 0.0, 125.0))
        return false;
    if (!_set_domain(mean_error, 0.0, 10.0, 0.35, 1.95))
        return false;
    if (!_set_domain(stacked, 0.0, 10.0, -0.85, 31.85))
        return false;

    if (!dvz_panel_set_reserve(
            stacked, &(DvzPanelReserve){.left_px = 56.0f, .right_px = 16.0f}))
        return false;

    if (!_add_autocorrelogram(ctx->scene, correlogram) ||
        !_add_mean_error(ctx->scene, mean_error) ||
        !_add_stacked_traces(ctx->scene, stacked))
        return false;

    if (!_add_axes(correlogram, "lag (ms)", "coincidence count", true, 18.0f))
        return false;
    if (!_add_axes(mean_error, "time (s)", "response", true, 18.0f))
        return false;
    if (!_add_axes(stacked, "time (s)", NULL, false, 18.0f))
        return false;

    return dvz_scenario_panzoom(ctx, correlogram, NULL, DVZ_DIM_MASK_XY) != NULL &&
           dvz_scenario_panzoom(ctx, mean_error, NULL, DVZ_DIM_MASK_XY) != NULL &&
           dvz_scenario_panzoom(ctx, stacked, NULL, DVZ_DIM_MASK_X) != NULL;
}



/**
 * Return the scientific plotting scenario specification.
 *
 * @return scenario specification
 */
DvzScenarioSpec dvz_showcase_scientific_plotting_scenario(void)
{
    return (DvzScenarioSpec){
        .id = "scientific_plotting_workflow",
        .title = "scientific_plotting",
        .width = WIDTH,
        .height = HEIGHT,
        .fps = 60.0,
        .requirements = DVZ_SCENARIO_REQ_TEXT_VISUAL | DVZ_SCENARIO_REQ_CONTROLLER |
                        DVZ_SCENARIO_REQ_PANZOOM,
        .init = _scenario_init,
    };
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Run the scientific plotting showcase through the native scenario runner.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @return process exit code
 */
#ifndef DVZ_EXAMPLE_NO_MAIN
int main(int argc, char** argv)
{
    DvzScenarioSpec spec = dvz_showcase_scientific_plotting_scenario();
    return dvz_scenario_run_native_cli(&spec, argc, argv) == 0 ? 0 : 1;
}
#endif
