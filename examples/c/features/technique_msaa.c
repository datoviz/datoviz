/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* msaa - side-by-side internal multisample antialiasing on thin segments.
 *
 * Scenario: feature.msaa
 * Style: features, graphite_cyan, 1600x1200 capture target
 *
 * Build:  just example-c features/technique_msaa
 * Run:    ./build/examples/c/features/technique_msaa --live
 * Smoke:  ./build/examples/c/features/technique_msaa --png
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
#define SEGMENT_COUNT 28u

static const float TAU = 6.28318530718f;



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Add a starburst of thin diagonal lines to a panel.
 *
 * @param scene scene owning the visual
 * @param panel panel receiving the visual
 * @return true on success
 */
static bool _add_line_burst(DvzScene* scene, DvzPanel* panel)
{
    vec3 starts[SEGMENT_COUNT] = {{0}};
    vec3 ends[SEGMENT_COUNT] = {{0}};
    DvzColor colors[SEGMENT_COUNT] = {{0}};
    float widths[SEGMENT_COUNT] = {0};

    for (uint32_t i = 0; i < SEGMENT_COUNT; i++)
    {
        const float t = (float)i / (float)SEGMENT_COUNT;
        const float a = TAU * t;
        starts[i][0] = -0.07f * cosf(a);
        starts[i][1] = -0.07f * sinf(a);
        starts[i][2] = 0.0f;
        ends[i][0] = +0.94f * cosf(a);
        ends[i][1] = +0.94f * sinf(a);
        ends[i][2] = 0.0f;
        colors[i] = example_graphite_cyan_color(
            i % 3u == 0   ? EXAMPLE_STYLE_COLOR_ACCENT_PRIMARY
            : i % 3u == 1 ? EXAMPLE_STYLE_COLOR_ACCENT_SECONDARY
                          : EXAMPLE_STYLE_COLOR_WARNING);
        widths[i] = 1.25f;
    }

    DvzVisual* segment = dvz_segment(scene, 0);
    if (segment == NULL)
        return false;
    DvzVisualDataUpdate updates[] = {
        {.attr_name = "position_start", .data = starts, .item_count = SEGMENT_COUNT},
        {.attr_name = "position_end", .data = ends, .item_count = SEGMENT_COUNT},
        {.attr_name = "color", .data = colors, .item_count = SEGMENT_COUNT},
        {.attr_name = "stroke_width", .data = widths, .item_count = SEGMENT_COUNT},
    };
    if (dvz_visual_set_data_many(segment, updates, 4) != 0)
        return false;
    if (dvz_visual_set_depth_test(segment, false) != 0)
        return false;
    return dvz_panel_add_visual(panel, segment, NULL) == 0;
}



/*************************************************************************************************/
/*  Scenario callbacks                                                                           */
/*************************************************************************************************/

/**
 * Initialize the MSAA feature scenario.
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

    DvzGrid* grid = dvz_figure_grid(ctx->figure, 1, 2);
    if (grid == NULL)
        return false;
    if (!dvz_grid_set_margins(
            grid, &(DvzPanelReserve){
                      .left_px = 42.0f, .right_px = 42.0f, .top_px = 38.0f, .bottom_px = 38.0f}))
        return false;
    if (!dvz_grid_set_gutter(grid, 30.0f, 0.0f))
        return false;

    DvzPanel* single = dvz_grid_panel(grid, 0, 0);
    DvzPanel* multisample = dvz_grid_panel(grid, 0, 1);
    if (single == NULL || multisample == NULL)
        return false;
    example_graphite_cyan_set_panel_background(single);
    example_graphite_cyan_set_panel_background(multisample);

    if (!_add_line_burst(ctx->scene, single) || !_add_line_burst(ctx->scene, multisample))
        return false;

    DvzMsaaDesc msaa = dvz_msaa_desc();
    msaa.enabled = true;
    msaa.sample_count = 4u;
    msaa.alpha_to_coverage = false;
    return dvz_panel_set_msaa(multisample, &msaa);
}



/**
 * Return the MSAA scenario specification.
 *
 * @return scenario specification
 */
static DvzScenarioSpec _msaa_scenario(void)
{
    return (DvzScenarioSpec){
        .id = "technique_msaa",
        .title = "msaa",
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
 * Run the MSAA feature example through the native scenario runner.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @return process exit code
 */
int main(int argc, char** argv)
{
    DvzScenarioSpec spec = _msaa_scenario();
    return dvz_scenario_run_native_cli(&spec, argc, argv) == 0 ? 0 : 1;
}
