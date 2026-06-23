/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* visual_transform - retained visual-local affine transform on point visuals.
 *
 * Scenario: feature.visual_transform
 * Style: features, graphite_cyan, 1600x1200 capture target
 *
 * Build:  just example-c features/visual_transform
 * Run:    ./build/examples/c/features/visual_transform --live
 * Smoke:  ./build/examples/c/features/visual_transform --png
 */



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>

#include "_assertions.h"
#include "datoviz/scene.h"
#include "example_style.h"
#include "runner/scenario_runner.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH       1600u
#define HEIGHT      1200u
#define POINT_COUNT 5u



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Upload one retained point visual.
 *
 * @param visual point visual
 * @param color point color
 * @return true when data and style were accepted
 */
static bool _upload_points(DvzVisual* visual, DvzColor color)
{
    ANN(visual);

    const vec3 positions[POINT_COUNT] = {
        {-0.46f, -0.24f, 0.0f}, {-0.18f, +0.24f, 0.0f}, {+0.00f, -0.08f, 0.0f},
        {+0.24f, +0.30f, 0.0f}, {+0.46f, -0.18f, 0.0f},
    };
    DvzColor colors[POINT_COUNT] = {{0}};
    float diameters[POINT_COUNT] = {28.0f, 40.0f, 24.0f, 44.0f, 30.0f};
    for (uint32_t i = 0; i < POINT_COUNT; i++)
        colors[i] = color;

    DvzVisualDataUpdate updates[] = {
        {.attr_name = "position", .data = positions, .item_count = POINT_COUNT},
        {.attr_name = "color", .data = colors, .item_count = POINT_COUNT},
        {.attr_name = "diameter_px", .data = diameters, .item_count = POINT_COUNT},
    };
    if (dvz_visual_set_data_many(visual, updates, 3) != 0)
        return false;

    DvzPointStyleDesc style = dvz_point_style_desc();
    style.aspect = DVZ_SHAPE_ASPECT_FILLED;
    style.stroke_width_px = 0.0f;
    if (dvz_point_set_style(visual, &style) != 0)
        return false;
    return dvz_visual_set_depth_test(visual, false) == 0;
}



/**
 * Return a simple affine transform with translation and non-uniform scale.
 *
 * @param x X translation
 * @param y Y translation
 * @param out output matrix
 */
static void _example_transform(float x, float y, mat4 out)
{
    ANN(out);

    out[0][0] = 1.24f;
    out[0][1] = 0.22f;
    out[0][2] = 0.0f;
    out[0][3] = 0.0f;
    out[1][0] = -0.18f;
    out[1][1] = 0.82f;
    out[1][2] = 0.0f;
    out[1][3] = 0.0f;
    out[2][0] = 0.0f;
    out[2][1] = 0.0f;
    out[2][2] = 1.0f;
    out[2][3] = 0.0f;
    out[3][0] = x;
    out[3][1] = y;
    out[3][2] = 0.0f;
    out[3][3] = 1.0f;
}


/**
 * Add one point visual to a panel, optionally with a visual-local transform.
 *
 * @param scene scene owning the visual
 * @param panel panel receiving the visual
 * @param transformed whether to apply the example transform
 * @return true on success
 */
static bool _add_panel_points(DvzScene* scene, DvzPanel* panel, bool transformed)
{
    ANN(scene);
    ANN(panel);

    DvzVisual* visual = dvz_point(scene, 0);
    if (visual == NULL)
        return false;

    DvzColor color = transformed ? example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ACCENT_PRIMARY)
                                 : example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_GRID);
    if (!transformed)
        color.a = 210u;
    if (!_upload_points(visual, color))
        return false;

    if (transformed)
    {
        mat4 transform = {{0}};
        mat4 current = {{0}};
        _example_transform(0.16f, 0.18f, transform);
        if (dvz_visual_set_transform(visual, transform) != 0)
            return false;
        if (!dvz_visual_has_transform(visual))
            return false;
        if (dvz_visual_get_transform(visual, current) != 0)
            return false;
    }

    return dvz_panel_add_visual(panel, visual, NULL) == 0;
}



/*************************************************************************************************/
/*  Scenario callbacks                                                                           */
/*************************************************************************************************/

/**
 * Initialize the visual-transform feature example.
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
    if (!dvz_grid_set_margins(grid, &(DvzPanelReserve){.left_px = 36, .right_px = 36,
                                                       .top_px = 36, .bottom_px = 36}))
        return false;
    if (!dvz_grid_set_gutter(grid, 36.0f, 0.0f))
        return false;

    DvzPanel* base = dvz_grid_panel(grid, 0, 0);
    DvzPanel* transformed = dvz_grid_panel(grid, 0, 1);
    if (base == NULL || transformed == NULL)
        return false;
    example_graphite_cyan_set_panel_background(base);
    example_graphite_cyan_set_panel_background(transformed);

    if (!_add_panel_points(ctx->scene, base, false))
        return false;
    if (!_add_panel_points(ctx->scene, transformed, true))
        return false;

    return dvz_scenario_panzoom(ctx, base, NULL, DVZ_DIM_MASK_XY) != NULL &&
           dvz_scenario_panzoom(ctx, transformed, NULL, DVZ_DIM_MASK_XY) != NULL;
}



/**
 * Return the visual-transform scenario specification.
 *
 * @return scenario specification
 */
static DvzScenarioSpec _visual_transform_scenario(void)
{
    return (DvzScenarioSpec){
        .id = "feature_visual_transform",
        .title = "visual_transform",
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
 * Run the visual-transform feature example through the native scenario runner.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @return process exit code
 */
int main(int argc, char** argv)
{
    DvzScenarioSpec spec = _visual_transform_scenario();
    return dvz_scenario_run_native_cli(&spec, argc, argv) == 0 ? 0 : 1;
}
