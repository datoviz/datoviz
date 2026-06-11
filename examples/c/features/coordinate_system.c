/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* coordinate_system - visual proof of the Datoviz scene coordinate convention.
 *
 * Scenario: feature.coordinate_system
 * Style: features, graphite_cyan, 1600x1200 capture target
 *
 * Build:  just example-c features/coordinate_system
 * Run:    ./build/examples/c/features/coordinate_system --live
 * Smoke:  ./build/examples/c/features/coordinate_system --png
 *
 * The right-hand point is +X, the top point is +Y, and the center cyan point marks +Z in front
 * of the larger yellow -Z point at the same X/Y location.
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

#define WIDTH         1600u
#define HEIGHT        1200u
#define AXIS_SEGMENTS 6u
#define AXIS_POINTS   4u
#define DEPTH_POINTS 1u



/*************************************************************************************************/
/*  Forward declarations                                                                         */
/*************************************************************************************************/

DvzScenarioSpec dvz_example_coordinate_system_scenario(void);



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Add a screen-placed text label.
 *
 * @param panel target panel
 * @param string label text
 * @param x screen X coordinate in logical pixels
 * @param y screen Y coordinate in logical pixels
 * @param size text size in logical pixels
 * @param role graphite-cyan color role
 * @return true when the label was added
 */
static bool _add_label(
    DvzPanel* panel, const char* string, float x, float y, float size,
    ExampleStyleColorRole role)
{
    DvzText* text = dvz_text(panel, 0);
    if (text == NULL)
        return false;

    DvzColor color = example_graphite_cyan_color(role);
    DvzTextStyle style = dvz_text_style();
    style.size_px = size;
    style.renderer = DVZ_TEXT_RENDERER_MSDF_ATLAS;
    style.color[0] = color.r;
    style.color[1] = color.g;
    style.color[2] = color.b;
    style.color[3] = color.a;
    if (dvz_text_set_style(text, &style) != 0)
        return false;

    DvzTextPlacement placement = dvz_text_placement();
    placement.mode = DVZ_TEXT_PLACEMENT_SCREEN;
    placement.anchor = DVZ_SCENE_ANCHOR_PANEL_TOP_LEFT;
    placement.position[0] = x;
    placement.position[1] = y;
    placement.position[2] = 0.0f;
    placement.text_anchor[0] = 0.5f;
    placement.text_anchor[1] = 0.5f;
    placement.has_text_anchor = true;
    dvz_text_set_placement(text, &placement);
    dvz_text_set_string(text, string);
    return true;
}



/**
 * Add X/Y axes and arrow hints in scene coordinates.
 *
 * @param scene scene owning the visual
 * @param panel panel receiving the visual
 * @return true when all segments were added
 */
static bool _add_xy_axes(DvzScene* scene, DvzPanel* panel)
{
    const vec3 starts[AXIS_SEGMENTS] = {
        {-0.82f, 0.00f, 0.0f},
        {0.00f, -0.66f, 0.0f},
        {0.72f, 0.00f, 0.0f},
        {0.72f, 0.00f, 0.0f},
        {0.00f, 0.56f, 0.0f},
        {0.00f, 0.56f, 0.0f},
    };
    const vec3 ends[AXIS_SEGMENTS] = {
        {+0.82f, 0.00f, 0.0f},
        {0.00f, +0.66f, 0.0f},
        {0.62f, +0.08f, 0.0f},
        {0.62f, -0.08f, 0.0f},
        {+0.08f, 0.46f, 0.0f},
        {-0.08f, 0.46f, 0.0f},
    };
    DvzColor colors[AXIS_SEGMENTS] = {0};
    float widths[AXIS_SEGMENTS] = {0};
    for (uint32_t i = 0; i < AXIS_SEGMENTS; i++)
    {
        colors[i] = example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_GRID);
        colors[i].a = 230;
        widths[i] = i < 2u ? 8.0f : 6.0f;
    }

    DvzVisual* visual = dvz_segment(scene, 0);
    if (visual == NULL)
        return false;

    DvzVisualDataUpdate updates[] = {
        {.attr_name = "position_start", .data = starts, .item_count = AXIS_SEGMENTS},
        {.attr_name = "position_end", .data = ends, .item_count = AXIS_SEGMENTS},
        {.attr_name = "color", .data = colors, .item_count = AXIS_SEGMENTS},
        {.attr_name = "stroke_width", .data = widths, .item_count = AXIS_SEGMENTS},
    };
    if (dvz_visual_set_data_many(visual, updates, 4) != 0)
        return false;
    if (dvz_segment_set_caps(visual, DVZ_SEGMENT_CAP_ROUND, DVZ_SEGMENT_CAP_ROUND) != 0)
        return false;
    if (dvz_visual_set_depth_test(visual, false) != 0)
        return false;
    return dvz_panel_add_visual(panel, visual, NULL) == 0;
}



/**
 * Add endpoint markers for the X/Y directions.
 *
 * @param scene scene owning the visual
 * @param panel panel receiving the visual
 * @return true when the markers were added
 */
static bool _add_xy_markers(DvzScene* scene, DvzPanel* panel)
{
    const vec3 positions[AXIS_POINTS] = {
        {-0.72f, 0.00f, 0.0f},
        {+0.72f, 0.00f, 0.0f},
        {0.00f, -0.56f, 0.0f},
        {0.00f, +0.56f, 0.0f},
    };
    DvzColor colors[AXIS_POINTS] = {
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_MINOR_TICK),
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ACCENT_PRIMARY),
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_MINOR_TICK),
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ACCENT_SECONDARY),
    };
    const float diameters[AXIS_POINTS] = {44.0f, 58.0f, 44.0f, 58.0f};

    DvzVisual* point = dvz_point(scene, 0);
    if (point == NULL)
        return false;

    DvzVisualDataUpdate updates[] = {
        {.attr_name = "position", .data = positions, .item_count = AXIS_POINTS},
        {.attr_name = "color", .data = colors, .item_count = AXIS_POINTS},
        {.attr_name = "diameter", .data = diameters, .item_count = AXIS_POINTS},
    };
    if (dvz_visual_set_data_many(point, updates, 3) != 0)
        return false;

    DvzPointStyleDesc style = dvz_point_style_desc();
    style.aspect = DVZ_SHAPE_ASPECT_FILLED;
    style.stroke_width = 0.0f;
    if (dvz_point_set_style(point, &style) != 0)
        return false;
    if (dvz_visual_set_depth_test(point, false) != 0)
        return false;
    return dvz_panel_add_visual(panel, point, NULL) == 0;
}



/**
 * Add one point used by the overlapping Z front/back proof.
 *
 * @param scene scene owning the visual
 * @param panel panel receiving the visual
 * @param position point center
 * @param diameter point diameter in pixels
 * @param color point color
 * @param z_layer draw layer
 * @return true when the point was added
 */
static bool _add_z_point(
    DvzScene* scene, DvzPanel* panel, const vec3 position, float diameter, DvzColor color,
    int32_t z_layer)
{
    vec3 positions[DEPTH_POINTS] = {{position[0], position[1], position[2]}};
    float diameters[DEPTH_POINTS] = {diameter};
    DvzColor colors[DEPTH_POINTS] = {color};

    DvzVisual* point = dvz_point(scene, 0);
    if (point == NULL)
        return false;
    DvzVisualDataUpdate updates[] = {
        {.attr_name = "position", .data = positions, .item_count = DEPTH_POINTS},
        {.attr_name = "color", .data = colors, .item_count = DEPTH_POINTS},
        {.attr_name = "diameter", .data = diameters, .item_count = DEPTH_POINTS},
    };
    if (dvz_visual_set_data_many(point, updates, 3) != 0)
        return false;

    DvzPointStyleDesc style = dvz_point_style_desc();
    style.aspect = DVZ_SHAPE_ASPECT_FILLED;
    style.stroke_width = 0.0f;
    if (dvz_point_set_style(point, &style) != 0)
        return false;
    if (dvz_visual_set_depth_test(point, false) != 0)
        return false;

    DvzVisualAttachDesc attach = dvz_visual_attach_desc();
    attach.z_layer = z_layer;
    return dvz_panel_add_visual(panel, point, &attach) == 0;
}



/**
 * Add an overlapping visual pair that marks +Z as the front direction.
 *
 * @param scene scene owning the visual
 * @param panel panel receiving the visual
 * @return true when the points were added
 */
static bool _add_z_depth_pair(DvzScene* scene, DvzPanel* panel)
{
    DvzColor front = example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ACCENT_PRIMARY);
    DvzColor back = example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_WARNING);
    return _add_z_point(scene, panel, (vec3){0.0f, 0.0f, -0.24f}, 260.0f, back, 0) &&
           _add_z_point(scene, panel, (vec3){0.0f, 0.0f, +0.24f}, 170.0f, front, 1);
}



/**
 * Add labels that make the visual inspection unambiguous.
 *
 * @param panel target panel
 * @return true when all labels were added
 */
static bool _add_labels(DvzPanel* panel)
{
    return _add_label(
               panel, "Scene coordinates", 800.0f, 112.0f, 64.0f,
               EXAMPLE_STYLE_COLOR_TEXT) &&
           _add_label(
               panel, "-X left", 360.0f, 600.0f, 38.0f,
               EXAMPLE_STYLE_COLOR_MINOR_TICK) &&
           _add_label(
               panel, "+X right", 1240.0f, 600.0f, 42.0f,
               EXAMPLE_STYLE_COLOR_ACCENT_PRIMARY) &&
           _add_label(
               panel, "-Y bottom", 800.0f, 930.0f, 38.0f,
               EXAMPLE_STYLE_COLOR_MINOR_TICK) &&
           _add_label(
               panel, "+Y top", 800.0f, 265.0f, 42.0f,
               EXAMPLE_STYLE_COLOR_ACCENT_SECONDARY) &&
           _add_label(
               panel, "+Z front", 1030.0f, 515.0f, 42.0f,
               EXAMPLE_STYLE_COLOR_ACCENT_PRIMARY) &&
           _add_label(
               panel, "-Z back", 570.0f, 690.0f, 34.0f,
               EXAMPLE_STYLE_COLOR_WARNING);
}



/*************************************************************************************************/
/*  Scenario callbacks                                                                           */
/*************************************************************************************************/

/**
 * Initialize the coordinate-system proof scenario.
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

    return _add_xy_axes(ctx->scene, panel) && _add_xy_markers(ctx->scene, panel) &&
           _add_z_depth_pair(ctx->scene, panel) && _add_labels(panel);
}



/**
 * Return the coordinate-system scenario specification.
 *
 * @return scenario specification
 */
DvzScenarioSpec dvz_example_coordinate_system_scenario(void)
{
    return (DvzScenarioSpec){
        .id = "feature_coordinate_system",
        .title = "coordinate_system",
        .width = WIDTH,
        .height = HEIGHT,
        .fps = 60.0,
        .requirements = DVZ_SCENARIO_REQ_POINT_VISUAL | DVZ_SCENARIO_REQ_TEXT_VISUAL,
        .init = _scenario_init,
    };
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Run the coordinate-system feature example through the native scenario runner.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @return process exit code
 */
#ifndef DVZ_EXAMPLE_NO_MAIN
int main(int argc, char** argv)
{
    DvzScenarioSpec spec = dvz_example_coordinate_system_scenario();
    return dvz_scenario_run_native_cli(&spec, argc, argv) == 0 ? 0 : 1;
}
#endif
