/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* edl - Eye-Dome Lighting applied to a dense retained point cloud.
 *
 * Scenario: feature.edl
 * Style: features, graphite_cyan, 1600x1200 capture target
 *
 * Build:  just example-c features/technique_edl
 * Run:    ./build/examples/c/features/technique_edl --live
 * Smoke:  ./build/examples/c/features/technique_edl --png
 */



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>
#include <math.h>

#include "datoviz/scene.h"
#include "example_common.h"
#include "example_style.h"
#include "runner/scenario_runner.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH       1600u
#define HEIGHT      1200u
#define POINT_COUNT 192u

static const float TAU = 6.28318530718f;



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Add one dense depth-rich point cloud to a panel.
 *
 * @param scene scene owning the visual
 * @param panel panel receiving the visual
 * @return true on success
 */
static bool _add_depth_cloud(DvzScene* scene, DvzPanel* panel)
{
    vec3 positions[POINT_COUNT] = {{0}};
    DvzColor colors[POINT_COUNT] = {{0}};
    float sizes[POINT_COUNT] = {0};

    for (uint32_t i = 0; i < POINT_COUNT; i++)
    {
        const float layer = (float)(i % 12u);
        const float row = (float)(i / 12u);
        const float t = POINT_COUNT > 1u ? (float)i / (float)(POINT_COUNT - 1u) : 0.0f;
        const float theta = TAU * (0.19f * row + 0.083333f * layer);
        const float radius = 0.16f + 0.82f * sqrtf(t);
        const float wave = 0.34f * sinf(TAU * (3.0f * t + 0.083333f * layer));

        positions[i][0] = 0.78f * radius * cosf(theta);
        positions[i][1] = 0.68f * radius * sinf(theta);
        positions[i][2] = -0.80f + 1.60f * (layer / 11.0f) + wave;

        DvzColor color = example_graphite_cyan_color(
            layer < 4.0f   ? EXAMPLE_STYLE_COLOR_ACCENT_PRIMARY
            : layer < 8.0f ? EXAMPLE_STYLE_COLOR_ACCENT_SECONDARY
                            : EXAMPLE_STYLE_COLOR_WARNING);
        color.a = 255u;
        colors[i] = color;
        sizes[i] = 22.0f + 12.0f * (0.5f + 0.5f * sinf(TAU * (4.0f * t)));
    }

    DvzVisual* point = dvz_point(scene, 0);
    if (point == NULL)
        return false;

    DvzVisualDataUpdate updates[] = {
        {.attr_name = "position", .data = positions, .item_count = POINT_COUNT},
        {.attr_name = "color", .data = colors, .item_count = POINT_COUNT},
        {.attr_name = "size", .data = sizes, .item_count = POINT_COUNT},
    };
    if (dvz_visual_set_data_many(point, updates, 3) != 0)
        return false;
    return dvz_panel_add_visual(panel, point, NULL) == 0;
}



/*************************************************************************************************/
/*  Scenario callbacks                                                                           */
/*************************************************************************************************/

/**
 * Initialize the EDL feature scenario.
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

    DvzPanel* plain = dvz_grid_panel(grid, 0, 0);
    DvzPanel* lit = dvz_grid_panel(grid, 0, 1);
    if (plain == NULL || lit == NULL)
        return false;
    example_graphite_cyan_set_panel_background(plain);
    example_graphite_cyan_set_panel_background(lit);
    if (!example_add_large_panel_label(plain, "plain depth") ||
        !example_add_large_panel_label(lit, "EDL resolve"))
        return false;

    if (!_add_depth_cloud(ctx->scene, plain) || !_add_depth_cloud(ctx->scene, lit))
        return false;

    DvzEdlDesc edl = dvz_edl_desc();
    edl.radius = 8.0f;
    edl.strength = 200.0f;
    edl.depth_scale = 1000.0f;
    return dvz_panel_set_edl(lit, &edl);
}



/**
 * Return the EDL scenario specification.
 *
 * @return scenario specification
 */
static DvzScenarioSpec _edl_scenario(void)
{
    return (DvzScenarioSpec){
        .id = "technique_edl",
        .title = "edl",
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
 * Run the Eye-Dome Lighting feature example through the native scenario runner.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @return process exit code
 */
int main(int argc, char** argv)
{
    DvzScenarioSpec spec = _edl_scenario();
    return dvz_scenario_run_native_cli(&spec, argc, argv) == 0 ? 0 : 1;
}
