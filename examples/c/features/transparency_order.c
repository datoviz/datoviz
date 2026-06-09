/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* transparency_order - source-over and weighted blended order-independent transparency.
 *
 * Scenario: feature.transparency_order
 * Style: features, graphite_cyan, 1600x1200 capture target
 *
 * Build:  just example-c features/transparency_order
 * Run:    ./build/examples/c/features/transparency_order --live
 * Smoke:  ./build/examples/c/features/transparency_order --png
 */



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>

#include "datoviz/scene.h"
#include "example_style.h"
#include "runner/scenario_runner.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH        1600u
#define HEIGHT       1200u
#define VERTEX_COUNT 9u



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Fill three overlapping translucent triangles.
 *
 * @param positions output vertex positions
 * @param normals output normals
 * @param colors output colors
 */
static void _fill_triangles(
    vec3 positions[VERTEX_COUNT], vec3 normals[VERTEX_COUNT], DvzColor colors[VERTEX_COUNT])
{
    const vec3 data[VERTEX_COUNT] = {
        {-0.82f, -0.54f, 0.00f}, {-0.10f, +0.76f, 0.00f}, {+0.32f, -0.38f, 0.00f},
        {-0.26f, +0.54f, 0.18f}, {+0.84f, +0.22f, 0.18f}, {+0.02f, -0.82f, 0.18f},
        {-0.84f, +0.02f, 0.36f}, {+0.18f, +0.88f, 0.36f}, {+0.78f, -0.06f, 0.36f},
    };
    const ExampleStyleColorRole roles[3] = {
        EXAMPLE_STYLE_COLOR_ACCENT_PRIMARY,
        EXAMPLE_STYLE_COLOR_ACCENT_SECONDARY,
        EXAMPLE_STYLE_COLOR_WARNING,
    };
    const uint8_t alpha[3] = {122, 136, 150};

    for (uint32_t i = 0; i < VERTEX_COUNT; i++)
    {
        positions[i][0] = data[i][0];
        positions[i][1] = data[i][1];
        positions[i][2] = data[i][2];
        normals[i][0] = 0.0f;
        normals[i][1] = 0.0f;
        normals[i][2] = 1.0f;
        const uint32_t triangle = i / 3u;
        colors[i] = example_graphite_cyan_color(roles[triangle]);
        colors[i].a = alpha[triangle];
    }
}



/**
 * Add one transparent primitive visual.
 *
 * @param scene scene owning the visual
 * @param panel panel receiving the visual
 * @param mode alpha mode
 * @return true on success
 */
static bool _add_transparent_triangles(DvzScene* scene, DvzPanel* panel, DvzAlphaMode mode)
{
    vec3 positions[VERTEX_COUNT] = {{0}};
    vec3 normals[VERTEX_COUNT] = {{0}};
    DvzColor colors[VERTEX_COUNT] = {{0}};
    _fill_triangles(positions, normals, colors);

    DvzVisual* visual = dvz_primitive(scene, DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0);
    if (visual == NULL)
        return false;
    DvzVisualDataUpdate updates[] = {
        {.attr_name = "position", .data = positions, .item_count = VERTEX_COUNT},
        {.attr_name = "normal", .data = normals, .item_count = VERTEX_COUNT},
        {.attr_name = "color", .data = colors, .item_count = VERTEX_COUNT},
    };
    if (dvz_visual_set_data_many(visual, updates, 3) != 0)
        return false;
    if (dvz_visual_set_alpha_mode(visual, mode) != 0)
        return false;
    return dvz_panel_add_visual(panel, visual, NULL) == 0;
}



/*************************************************************************************************/
/*  Scenario callbacks                                                                           */
/*************************************************************************************************/

/**
 * Initialize the transparency-order feature scenario.
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

    DvzPanel* blended = dvz_grid_panel(grid, 0, 0);
    DvzPanel* wboit = dvz_grid_panel(grid, 0, 1);
    if (blended == NULL || wboit == NULL)
        return false;
    example_graphite_cyan_set_panel_background(blended);
    example_graphite_cyan_set_panel_background(wboit);

    return _add_transparent_triangles(ctx->scene, blended, DVZ_ALPHA_BLENDED) &&
           _add_transparent_triangles(ctx->scene, wboit, DVZ_ALPHA_WBOIT);
}



/**
 * Return the transparency-order scenario specification.
 *
 * @return scenario specification
 */
static DvzScenarioSpec _transparency_order_scenario(void)
{
    return (DvzScenarioSpec){
        .id = "feature_transparency_order",
        .title = "transparency_order",
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
 * Run the transparency-order feature example through the native scenario runner.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @return process exit code
 */
int main(int argc, char** argv)
{
    DvzScenarioSpec spec = _transparency_order_scenario();
    return dvz_scenario_run_native_cli(&spec, argc, argv) == 0 ? 0 : 1;
}
