/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* path_join - acute stroked path joins for visual regression inspection.
 *
 * Scenario: feature_path_join
 * Style: features, graphite_cyan, 1600x1200 capture target
 *
 * Build:  just example-c features/path_join
 * Run:    ./build/examples/c/features/path_join --live
 * Smoke:  ./build/examples/c/features/path_join --png
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

#define WIDTH         1600u
#define HEIGHT        1200u
#define MAX_POINTS    96u
#define MAX_SUBPATHS  4u
#define JOIN_COUNT    3u
#define STAR_POINTS   5u
#define CLOSED_POINTS (2u * STAR_POINTS + 1u)

static const float TAU = 6.28318530718f;



/*************************************************************************************************/
/*  Function prototypes                                                                          */
/*************************************************************************************************/

DvzScenarioSpec dvz_example_path_join_scenario(void);



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Append one point to path arrays.
 *
 * @param positions output position array
 * @param colors output color array
 * @param widths output width array
 * @param count input/output point count
 * @param x x coordinate
 * @param y y coordinate
 * @param color point color
 * @param width stroke width
 */
static void _append_point(
    vec3 positions[MAX_POINTS], DvzColor colors[MAX_POINTS], float widths[MAX_POINTS],
    uint32_t* count, float x, float y, DvzColor color, float width)
{
    ANN(positions);
    ANN(colors);
    ANN(widths);
    ANN(count);
    ASSERT(*count < MAX_POINTS);

    positions[*count][0] = x;
    positions[*count][1] = y;
    positions[*count][2] = 0.0f;
    colors[*count] = color;
    widths[*count] = width;
    *count += 1;
}



/**
 * Append an acute V-shaped open path.
 *
 * @param positions output position array
 * @param colors output color array
 * @param widths output width array
 * @param count input/output point count
 * @param subpaths output subpath lengths
 * @param subpath_count input/output subpath count
 * @param cx center x coordinate
 * @param cy center y coordinate
 * @param color point color
 * @param width stroke width
 */
static void _append_v_path(
    vec3 positions[MAX_POINTS], DvzColor colors[MAX_POINTS], float widths[MAX_POINTS],
    uint32_t* count, uint32_t subpaths[MAX_SUBPATHS], uint32_t* subpath_count, float cx, float cy,
    DvzColor color, float width)
{
    const uint32_t first = *count;
    _append_point(positions, colors, widths, count, cx - 0.21f, cy - 0.11f, color, width);
    _append_point(positions, colors, widths, count, cx + 0.00f, cy + 0.16f, color, width);
    _append_point(positions, colors, widths, count, cx + 0.21f, cy - 0.11f, color, width);
    subpaths[*subpath_count] = *count - first;
    *subpath_count += 1;
}



/**
 * Append a compact acute zigzag path.
 *
 * @param positions output position array
 * @param colors output color array
 * @param widths output width array
 * @param count input/output point count
 * @param subpaths output subpath lengths
 * @param subpath_count input/output subpath count
 * @param cx center x coordinate
 * @param cy center y coordinate
 * @param color point color
 * @param width stroke width
 */
static void _append_zigzag_path(
    vec3 positions[MAX_POINTS], DvzColor colors[MAX_POINTS], float widths[MAX_POINTS],
    uint32_t* count, uint32_t subpaths[MAX_SUBPATHS], uint32_t* subpath_count, float cx, float cy,
    DvzColor color, float width)
{
    const uint32_t first = *count;
    const vec2 pts[] = {
        {-0.25f, -0.10f}, {-0.10f, +0.14f}, {+0.03f, -0.12f}, {+0.16f, +0.13f}, {+0.27f, -0.10f},
    };
    for (uint32_t i = 0; i < DVZ_ARRAY_COUNT(pts); i++)
        _append_point(
            positions, colors, widths, count, cx + pts[i][0], cy + pts[i][1], color, width);
    subpaths[*subpath_count] = *count - first;
    *subpath_count += 1;
}



/**
 * Append an open star polyline.
 *
 * @param positions output position array
 * @param colors output color array
 * @param widths output width array
 * @param count input/output point count
 * @param subpaths output subpath lengths
 * @param subpath_count input/output subpath count
 * @param cx center x coordinate
 * @param cy center y coordinate
 * @param color point color
 * @param width stroke width
 */
static void _append_open_star_path(
    vec3 positions[MAX_POINTS], DvzColor colors[MAX_POINTS], float widths[MAX_POINTS],
    uint32_t* count, uint32_t subpaths[MAX_SUBPATHS], uint32_t* subpath_count, float cx, float cy,
    DvzColor color, float width)
{
    const uint32_t first = *count;
    const uint32_t order[] = {0, 2, 4, 1, 3};
    for (uint32_t i = 0; i < DVZ_ARRAY_COUNT(order); i++)
    {
        const float a = -0.25f * TAU + TAU * (float)order[i] / (float)STAR_POINTS;
        _append_point(
            positions, colors, widths, count, cx + 0.21f * cosf(a), cy + 0.21f * sinf(a), color,
            width);
    }
    subpaths[*subpath_count] = *count - first;
    *subpath_count += 1;
}



/**
 * Append a closed star ring with a repeated first-point sentinel.
 *
 * @param positions output position array
 * @param colors output color array
 * @param widths output width array
 * @param count input/output point count
 * @param subpaths output subpath lengths
 * @param subpath_count input/output subpath count
 * @param cx center x coordinate
 * @param cy center y coordinate
 * @param color point color
 * @param width stroke width
 */
static void _append_closed_star_path(
    vec3 positions[MAX_POINTS], DvzColor colors[MAX_POINTS], float widths[MAX_POINTS],
    uint32_t* count, uint32_t subpaths[MAX_SUBPATHS], uint32_t* subpath_count, float cx, float cy,
    DvzColor color, float width)
{
    const uint32_t first = *count;
    for (uint32_t i = 0; i < CLOSED_POINTS; i++)
    {
        const uint32_t k = i % (2u * STAR_POINTS);
        const float radius = (k % 2u) == 0u ? 0.23f : 0.075f;
        const float a = -0.25f * TAU + TAU * (float)k / (float)(2u * STAR_POINTS);
        _append_point(
            positions, colors, widths, count, cx + radius * cosf(a), cy + radius * sinf(a), color,
            width);
    }
    subpaths[*subpath_count] = *count - first;
    *subpath_count += 1;
}



/**
 * Add one join-mode stress column.
 *
 * @param scene scene owning the visual
 * @param panel panel receiving the visual
 * @param join path join mode
 * @param cx column center x coordinate
 * @param color base stroke color
 * @return true when the visual was added
 */
static bool
_add_join_column(DvzScene* scene, DvzPanel* panel, DvzPathJoin join, float cx, DvzColor color)
{
    ANN(scene);
    ANN(panel);

    vec3 positions[MAX_POINTS] = {{0}};
    DvzColor colors[MAX_POINTS] = {{0}};
    float widths[MAX_POINTS] = {0};
    uint32_t subpaths[MAX_SUBPATHS] = {0};
    uint32_t count = 0;
    uint32_t subpath_count = 0;

    DvzColor translucent = color;
    translucent.a = 168;
    DvzColor opaque = color;
    opaque.a = 255;

    _append_v_path(
        positions, colors, widths, &count, subpaths, &subpath_count, cx, +0.61f, translucent,
        48.0f);
    _append_zigzag_path(
        positions, colors, widths, &count, subpaths, &subpath_count, cx, +0.19f, translucent,
        36.0f);
    _append_open_star_path(
        positions, colors, widths, &count, subpaths, &subpath_count, cx, -0.23f, opaque, 28.0f);
    _append_closed_star_path(
        positions, colors, widths, &count, subpaths, &subpath_count, cx, -0.67f, opaque, 22.0f);

    DvzVisual* visual = dvz_path(scene, 0);
    if (visual == NULL)
        return false;

    DvzVisualDataUpdate updates[] = {
        {.attr_name = "position", .data = positions, .item_count = count},
        {.attr_name = "color", .data = colors, .item_count = count},
        {.attr_name = "stroke_width", .data = widths, .item_count = count},
    };
    if (dvz_visual_set_data_many(visual, updates, 3) != 0)
        return false;
    if (dvz_path_set_subpaths(visual, subpath_count, subpaths) != 0)
        return false;
    if (dvz_path_set_caps(visual, DVZ_SEGMENT_CAP_ROUND, DVZ_SEGMENT_CAP_ROUND) != 0)
        return false;
    if (dvz_path_set_join(visual, join, 4.0f) != 0)
        return false;
    if (dvz_visual_set_alpha_mode(visual, DVZ_ALPHA_BLENDED) != 0)
        return false;
    if (dvz_visual_set_depth_test(visual, false) != 0)
        return false;

    return dvz_panel_add_visual(panel, visual, NULL) == 0;
}



/*************************************************************************************************/
/*  Scenario callbacks                                                                           */
/*************************************************************************************************/

/**
 * Initialize the path join scenario.
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
    dvz_panel_set_background_color(panel, dvz_color_from_unit(0.005f, 0.007f, 0.010f, 1.0f));

    const DvzPathJoin joins[JOIN_COUNT] = {
        DVZ_PATH_JOIN_MITER,
        DVZ_PATH_JOIN_ROUND,
        DVZ_PATH_JOIN_BEVEL,
    };
    const float xs[JOIN_COUNT] = {-0.63f, 0.0f, +0.63f};
    const ExampleStyleColorRole roles[JOIN_COUNT] = {
        EXAMPLE_STYLE_COLOR_ACCENT_PRIMARY,
        EXAMPLE_STYLE_COLOR_ACCENT_SECONDARY,
        EXAMPLE_STYLE_COLOR_WARNING,
    };

    for (uint32_t i = 0; i < JOIN_COUNT; i++)
    {
        if (!_add_join_column(
                ctx->scene, panel, joins[i], xs[i], example_graphite_cyan_color(roles[i])))
            return false;
    }
    return true;
}



/**
 * Return the path join scenario specification.
 *
 * @return scenario specification
 */
DvzScenarioSpec dvz_example_path_join_scenario(void)
{
    return (DvzScenarioSpec){
        .id = "feature_path_join",
        .title = "path_join",
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
 * Run the path join example through the native scenario runner.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @return process exit code
 */
#ifndef DVZ_EXAMPLE_NO_MAIN
int main(int argc, char** argv)
{
    DvzScenarioSpec spec = dvz_example_path_join_scenario();
    return dvz_scenario_run_native_cli(&spec, argc, argv) == 0 ? 0 : 1;
}
#endif
