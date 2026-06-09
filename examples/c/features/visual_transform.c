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
        {.attr_name = "diameter", .data = diameters, .item_count = POINT_COUNT},
    };
    if (dvz_visual_set_data_many(visual, updates, 3) != 0)
        return false;

    DvzPointStyleDesc style = dvz_point_style_desc();
    style.aspect = DVZ_SHAPE_ASPECT_FILLED;
    style.stroke_width = 0.0f;
    if (dvz_point_set_style(visual, &style) != 0)
        return false;
    return dvz_visual_set_depth_test(visual, false) == 0;
}



/**
 * Return a simple translation transform.
 *
 * @param x X translation
 * @param y Y translation
 * @param out output matrix
 */
static void _translation(float x, float y, mat4 out)
{
    ANN(out);

    out[0][0] = 1.0f;
    out[0][1] = 0.0f;
    out[0][2] = 0.0f;
    out[0][3] = 0.0f;
    out[1][0] = 0.0f;
    out[1][1] = 1.0f;
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

    DvzPanel* panel = dvz_panel_full(ctx->figure);
    if (panel == NULL)
        return false;
    example_graphite_cyan_set_panel_background(panel);

    DvzVisual* base = dvz_point(ctx->scene, 0);
    DvzVisual* shifted = dvz_point(ctx->scene, 0);
    if (base == NULL || shifted == NULL)
        return false;

    DvzColor muted = example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_GRID);
    muted.a = 150u;
    if (!_upload_points(base, muted))
        return false;
    if (!_upload_points(shifted, example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ACCENT_PRIMARY)))
        return false;

    mat4 transform = {{0}};
    mat4 current = {{0}};
    _translation(0.36f, 0.30f, transform);
    if (dvz_visual_set_transform(shifted, transform) != 0)
        return false;
    if (!dvz_visual_has_transform(shifted))
        return false;
    if (dvz_visual_get_transform(shifted, current) != 0)
        return false;

    _translation(-0.38f, -0.28f, transform);
    if (dvz_visual_set_transform(base, transform) != 0)
        return false;
    if (dvz_visual_clear_transform(base) != 0 || dvz_visual_has_transform(base))
        return false;

    return dvz_panel_add_visual(panel, base, NULL) == 0 &&
           dvz_panel_add_visual(panel, shifted, NULL) == 0;
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
