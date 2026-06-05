/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* segment - retained segment visual with independent endpoint-pair strokes.
 *
 * Scenario: visual.segment
 * Style: visuals, graphite_cyan, 1600x1200 capture target
 *
 * Build:  just example-c visuals/segment
 * Run:    ./build/examples/c/visuals/segment --live
 * Smoke:  ./build/examples/c/visuals/segment --png
 */



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <math.h>
#include <stdbool.h>
#include <stdint.h>

#include "datoviz/scene.h"
#include "example_style.h"
#include "runner/scenario_runner.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH         1600u
#define HEIGHT        1200u
#define SEGMENT_COUNT 24u

static const float TAU = 6.28318530718f;



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Blend two color channels.
 *
 * @param a first channel
 * @param b second channel
 * @param t blend factor in [0, 1]
 * @return blended channel
 */
static uint8_t _mix_channel(uint8_t a, uint8_t b, float t)
{
    return (uint8_t)((1.0f - t) * (float)a + t * (float)b + 0.5f);
}



/**
 * Return one deterministic segment color.
 *
 * @param index segment index
 * @return segment color
 */
static DvzColor _segment_color(uint32_t index)
{
    DvzColor primary = example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ACCENT_PRIMARY);
    DvzColor secondary = example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ACCENT_SECONDARY);
    DvzColor warning = example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_WARNING);

    const uint32_t phase = index % 8u;
    const float t = (float)phase / 7.0f;
    DvzColor a = index % 3u == 2u ? warning : primary;
    DvzColor b = index % 3u == 0u ? secondary : primary;
    return dvz_color_rgba(
        _mix_channel(a.r, b.r, t), _mix_channel(a.g, b.g, t), _mix_channel(a.b, b.b, t), 232);
}



/**
 * Fill independent endpoint pairs arranged in three separated bands.
 *
 * @param starts output segment starts
 * @param ends output segment ends
 * @param colors output segment colors
 * @param widths output segment stroke widths
 */
static void _fill_segments(
    vec3 starts[SEGMENT_COUNT], vec3 ends[SEGMENT_COUNT], DvzColor colors[SEGMENT_COUNT],
    float widths[SEGMENT_COUNT])
{
    for (uint32_t i = 0; i < SEGMENT_COUNT; i++)
    {
        const uint32_t band = i / 8u;
        const uint32_t col = i % 8u;
        const float t = (float)col / 7.0f;
        const float cx = -0.82f + 1.64f * t;
        const float cy = 0.52f - 0.52f * (float)band;
        const float angle = (0.10f + 0.11f * (float)band + 0.07f * (float)(col % 3u)) * TAU;
        const float length = 0.13f + 0.08f * (float)((col + 2u * band) % 4u);
        const float dx = 0.5f * length * cosf(angle);
        const float dy = 0.5f * length * sinf(angle);

        starts[i][0] = cx - dx;
        starts[i][1] = cy - dy;
        starts[i][2] = 0.0f;
        ends[i][0] = cx + dx;
        ends[i][1] = cy + dy;
        ends[i][2] = 0.0f;
        colors[i] = _segment_color(i);
        widths[i] = 2.0f + 1.4f * (float)((i + band) % 5u);
    }
}



/*************************************************************************************************/
/*  Scenario callbacks                                                                           */
/*************************************************************************************************/

/**
 * Initialize the retained segment visual scenario.
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

    vec3 starts[SEGMENT_COUNT] = {{0}};
    vec3 ends[SEGMENT_COUNT] = {{0}};
    DvzColor colors[SEGMENT_COUNT] = {{0}};
    float widths[SEGMENT_COUNT] = {0};
    _fill_segments(starts, ends, colors, widths);

    ctx->figure = dvz_figure(ctx->scene, ctx->width, ctx->height, 0);
    if (ctx->figure == NULL)
        return false;

    DvzPanel* panel = dvz_panel_full(ctx->figure);
    if (panel == NULL)
        return false;
    example_graphite_cyan_set_panel_background(panel);

    DvzVisual* visual = dvz_segment(ctx->scene, 0);
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
    if (dvz_segment_set_caps(visual, DVZ_SEGMENT_CAP_ROUND, DVZ_SEGMENT_CAP_ROUND) != 0)
        return false;
    if (dvz_panel_add_visual(panel, visual, NULL) != 0)
        return false;

    DvzPanzoom* panzoom = dvz_scenario_panzoom(ctx, panel, NULL, DVZ_DIM_MASK_XY);
    return panzoom != NULL;
}



/**
 * Return the segment visual scenario specification.
 *
 * @return scenario specification
 */
static DvzScenarioSpec _segment_scenario(void)
{
    return (DvzScenarioSpec){
        .id = "visual_segment",
        .title = "visual_segment",
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
 * Run the retained segment visual example through the native scenario runner.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @return process exit code
 */
int main(int argc, char** argv)
{
    DvzScenarioSpec spec = _segment_scenario();
    return dvz_scenario_run_native_cli(&spec, argc, argv) == 0 ? 0 : 1;
}
