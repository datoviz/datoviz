/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* bezier_curve_path - tessellated cubic Bezier curve rendered as a retained path.
 *
 * Scenario: feature.bezier_curve_path
 * Style: features, graphite_cyan, 1600x1200 capture target
 *
 * Build:  just example-c features/bezier_curve_path
 * Run:    ./build/examples/c/features/bezier_curve_path --live
 * Smoke:  ./build/examples/c/features/bezier_curve_path --png
 */



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>

#include "_assertions.h"
#include "datoviz/geom.h"
#include "datoviz/scene.h"
#include "example_style.h"
#include "runner/scenario_runner.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH         1600u
#define HEIGHT        1200u
#define CONTROL_COUNT 4u
#define CONTROL_EDGES 3u



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Add the tessellated Bezier path.
 *
 * @param scene scene owning the visual
 * @param panel target panel
 * @param tess tessellated path
 * @return true when the curve was added
 */
static bool _add_curve(DvzScene* scene, DvzPanel* panel, const DvzTessellatedPath* tess)
{
    ANN(scene);
    ANN(panel);
    ANN(tess);

    if (tess->point_count == 0u || tess->points == NULL)
        return false;

    vec3 positions[64] = {{0}};
    DvzColor colors[64] = {{0}};
    float widths[64] = {0};
    if (tess->point_count > DVZ_ARRAY_COUNT(positions))
        return false;

    for (uint32_t i = 0; i < tess->point_count; i++)
    {
        positions[i][0] = (float)tess->points[i][0];
        positions[i][1] = (float)tess->points[i][1];
        positions[i][2] = (float)tess->points[i][2];
        colors[i] = example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ACCENT_PRIMARY);
        widths[i] = 8.0f;
    }

    DvzVisual* path = dvz_path(scene, 0);
    if (path == NULL)
        return false;
    DvzVisualDataUpdate updates[] = {
        {.attr_name = "position", .data = positions, .item_count = tess->point_count},
        {.attr_name = "color", .data = colors, .item_count = tess->point_count},
        {.attr_name = "stroke_width_px", .data = widths, .item_count = tess->point_count},
    };
    if (dvz_visual_set_data_many(path, updates, 3) != 0)
        return false;
    if (dvz_path_set_caps(path, DVZ_SEGMENT_CAP_ROUND, DVZ_SEGMENT_CAP_ROUND) != 0)
        return false;
    if (dvz_path_set_join(path, DVZ_PATH_JOIN_ROUND, 4.0f) != 0)
        return false;
    return dvz_panel_add_visual(panel, path, NULL) == 0;
}



/**
 * Add the Bezier control polygon as thin segments.
 *
 * @param scene scene owning the visual
 * @param panel target panel
 * @param controls cubic control points
 * @return true when the control polygon was added
 */
static bool
_add_control_polygon(DvzScene* scene, DvzPanel* panel, const dvec3 controls[CONTROL_COUNT])
{
    ANN(scene);
    ANN(panel);
    ANN(controls);

    vec3 starts[CONTROL_EDGES] = {{0}};
    vec3 ends[CONTROL_EDGES] = {{0}};
    DvzColor colors[CONTROL_EDGES] = {{0}};
    float widths[CONTROL_EDGES] = {0};

    for (uint32_t i = 0; i < CONTROL_EDGES; i++)
    {
        starts[i][0] = (float)controls[i][0];
        starts[i][1] = (float)controls[i][1];
        starts[i][2] = 0.0f;
        ends[i][0] = (float)controls[i + 1u][0];
        ends[i][1] = (float)controls[i + 1u][1];
        ends[i][2] = 0.0f;
        colors[i] = example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_GRID);
        colors[i].a = 180u;
        widths[i] = 2.0f;
    }

    DvzVisual* segment = dvz_segment(scene, 0);
    if (segment == NULL)
        return false;
    DvzVisualDataUpdate updates[] = {
        {.attr_name = "position_start", .data = starts, .item_count = CONTROL_EDGES},
        {.attr_name = "position_end", .data = ends, .item_count = CONTROL_EDGES},
        {.attr_name = "color", .data = colors, .item_count = CONTROL_EDGES},
        {.attr_name = "stroke_width_px", .data = widths, .item_count = CONTROL_EDGES},
    };
    if (dvz_visual_set_data_many(segment, updates, 4) != 0)
        return false;
    if (dvz_segment_set_caps(segment, DVZ_SEGMENT_CAP_ROUND, DVZ_SEGMENT_CAP_ROUND) != 0)
        return false;
    return dvz_panel_add_visual(panel, segment, NULL) == 0;
}



/**
 * Add the Bezier control points.
 *
 * @param scene scene owning the visual
 * @param panel target panel
 * @param controls cubic control points
 * @return true when the control points were added
 */
static bool
_add_control_points(DvzScene* scene, DvzPanel* panel, const dvec3 controls[CONTROL_COUNT])
{
    ANN(scene);
    ANN(panel);
    ANN(controls);

    vec3 positions[CONTROL_COUNT] = {{0}};
    DvzColor colors[CONTROL_COUNT] = {{0}};
    float diameters[CONTROL_COUNT] = {0};

    for (uint32_t i = 0; i < CONTROL_COUNT; i++)
    {
        positions[i][0] = (float)controls[i][0];
        positions[i][1] = (float)controls[i][1];
        positions[i][2] = 0.0f;
        colors[i] = i == 0u || i == CONTROL_COUNT - 1u
                        ? example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_WARNING)
                        : example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ACCENT_SECONDARY);
        diameters[i] = i == 0u || i == CONTROL_COUNT - 1u ? 34.0f : 24.0f;
    }

    DvzVisual* point = dvz_point(scene, 0);
    if (point == NULL)
        return false;
    DvzVisualDataUpdate updates[] = {
        {.attr_name = "position", .data = positions, .item_count = CONTROL_COUNT},
        {.attr_name = "color", .data = colors, .item_count = CONTROL_COUNT},
        {.attr_name = "diameter_px", .data = diameters, .item_count = CONTROL_COUNT},
    };
    if (dvz_visual_set_data_many(point, updates, 3) != 0)
        return false;
    return dvz_panel_add_visual(panel, point, NULL) == 0;
}



/*************************************************************************************************/
/*  Scenario callbacks                                                                           */
/*************************************************************************************************/

/**
 * Initialize the Bezier-curve path feature example.
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

    const dvec3 controls[CONTROL_COUNT] = {
        {-0.82, -0.45, 0.0},
        {-0.42, +0.76, 0.0},
        {+0.38, -0.74, 0.0},
        {+0.82, +0.45, 0.0},
    };
    DvzBezierTessellationDesc desc = dvz_bezier_tessellation_desc();
    desc.segment_count = 48u;
    DvzTessellatedPath* tess =
        dvz_tessellate_cubic_bezier(controls[0], controls[1], controls[2], controls[3], &desc);
    if (tess == NULL)
        return false;

    const bool ok = _add_control_polygon(ctx->scene, panel, controls) &&
                    _add_curve(ctx->scene, panel, tess) &&
                    _add_control_points(ctx->scene, panel, controls);
    dvz_tessellated_path_destroy(tess);
    return ok;
}



/**
 * Return the Bezier-curve path scenario specification.
 *
 * @return scenario specification
 */
static DvzScenarioSpec _bezier_curve_path_scenario(void)
{
    return (DvzScenarioSpec){
        .id = "feature_bezier_curve_path",
        .title = "bezier_curve_path",
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
 * Run the Bezier-curve path feature example through the native scenario runner.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @return process exit code
 */
int main(int argc, char** argv)
{
    DvzScenarioSpec spec = _bezier_curve_path_scenario();
    return dvz_scenario_run_native_cli(&spec, argc, argv) == 0 ? 0 : 1;
}
