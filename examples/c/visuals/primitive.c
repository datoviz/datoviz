/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* primitive - topology-parametric triangles rendered with the retained primitive visual.
 *
 * Scenario: visual.primitive
 * Style: visuals, graphite_cyan, 1600x1200 capture target
 *
 * Build:  just example-c visuals/primitive
 * Run:    ./build/examples/c/visuals/primitive --live
 * Smoke:  ./build/examples/c/visuals/primitive --png
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

#define WIDTH  1600u
#define HEIGHT 1200u



/*************************************************************************************************/
/*  Forward declarations                                                                         */
/*************************************************************************************************/

DvzScenarioSpec dvz_visual_primitive_scenario(void);



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Upload positions, graphite-cyan color roles, and flat normals into one primitive visual.
 *
 * @param visual primitive visual
 * @param positions vertex positions
 * @param color_roles vertex color roles
 * @param vertex_count vertex count
 * @return true when upload succeeds
 */
static bool _upload_primitive(
    DvzVisual* visual, const vec3* positions, const ExampleStyleColorRole* color_roles,
    uint32_t vertex_count)
{
    ANN(visual);
    ANN(positions);
    ANN(color_roles);

    vec3 normals[8] = {{0}};
    DvzColor colors[8] = {{0}};
    ASSERT(vertex_count <= 8u);
    for (uint32_t i = 0; i < vertex_count; i++)
    {
        normals[i][2] = 1.0f;
        colors[i] = example_graphite_cyan_color(color_roles[i]);
    }

    DvzVisualDataUpdate updates[] = {
        {.attr_name = "position", .data = positions, .item_count = vertex_count},
        {.attr_name = "color", .data = colors, .item_count = vertex_count},
        {.attr_name = "normal", .data = normals, .item_count = vertex_count},
    };
    return dvz_visual_set_data_many(visual, updates, 3) == 0;
}



/**
 * Add one primitive topology sample to the panel.
 *
 * @param scene scene owning the visual
 * @param panel panel receiving the visual
 * @param topology primitive topology
 * @param positions vertex positions
 * @param color_roles vertex color roles
 * @param vertex_count vertex count
 * @return true when the visual was added
 */
static bool _add_primitive(
    DvzScene* scene, DvzPanel* panel, DvzPrimitiveTopology topology, const vec3* positions,
    const ExampleStyleColorRole* color_roles, uint32_t vertex_count)
{
    ANN(scene);
    ANN(panel);

    DvzVisual* visual = dvz_primitive(scene, topology, 0);
    if (visual == NULL)
        return false;
    if (!_upload_primitive(visual, positions, color_roles, vertex_count))
        return false;
    return dvz_panel_add_visual(panel, visual, NULL) == 0;
}



/**
 * Add three topology-parametric primitive visuals.
 *
 * @param scene scene owning visuals
 * @param panel target panel
 * @return true when all visuals were added
 */
static bool _add_primitives(DvzScene* scene, DvzPanel* panel)
{
    ANN(scene);
    ANN(panel);

    const vec3 list_positions[6] = {
        {-0.88f, -0.42f, 0.00f}, {-0.34f, -0.42f, 0.00f}, {-0.61f, +0.44f, 0.00f},
        {-0.96f, +0.10f, 0.03f}, {-0.47f, +0.58f, 0.03f}, {-0.20f, -0.06f, 0.03f},
    };
    const ExampleStyleColorRole list_color_roles[6] = {
        EXAMPLE_STYLE_COLOR_ACCENT_PRIMARY,  EXAMPLE_STYLE_COLOR_ACCENT_SECONDARY,
        EXAMPLE_STYLE_COLOR_WARNING,         EXAMPLE_STYLE_COLOR_ERROR,
        EXAMPLE_STYLE_COLOR_TEXT,            EXAMPLE_STYLE_COLOR_ACCENT_PRIMARY,
    };
    if (!_add_primitive(
            scene, panel, DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, list_positions, list_color_roles,
            6))
        return false;

    const vec3 strip_positions[6] = {
        {-0.12f, +0.08f, 0.00f}, {+0.10f, +0.74f, 0.00f}, {+0.28f, +0.18f, 0.02f},
        {+0.48f, +0.82f, 0.02f}, {+0.66f, +0.26f, 0.04f}, {+0.88f, +0.70f, 0.04f},
    };
    const ExampleStyleColorRole strip_color_roles[6] = {
        EXAMPLE_STYLE_COLOR_ERROR,            EXAMPLE_STYLE_COLOR_ACCENT_PRIMARY,
        EXAMPLE_STYLE_COLOR_ACCENT_SECONDARY, EXAMPLE_STYLE_COLOR_WARNING,
        EXAMPLE_STYLE_COLOR_ACCENT_PRIMARY,   EXAMPLE_STYLE_COLOR_TEXT,
    };
    if (!_add_primitive(
            scene, panel, DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP, strip_positions,
            strip_color_roles, 6))
        return false;

    const vec3 fan_positions[8] = {
        {+0.38f, -0.44f, 0.06f}, {+0.16f, -0.86f, 0.06f}, {+0.54f, -0.92f, 0.06f},
        {+0.88f, -0.70f, 0.06f}, {+0.92f, -0.34f, 0.06f}, {+0.70f, -0.02f, 0.06f},
        {+0.34f, +0.04f, 0.06f}, {+0.08f, -0.18f, 0.06f},
    };
    const ExampleStyleColorRole fan_color_roles[8] = {
        EXAMPLE_STYLE_COLOR_TEXT,             EXAMPLE_STYLE_COLOR_ACCENT_PRIMARY,
        EXAMPLE_STYLE_COLOR_ACCENT_SECONDARY, EXAMPLE_STYLE_COLOR_WARNING,
        EXAMPLE_STYLE_COLOR_ERROR,            EXAMPLE_STYLE_COLOR_ACCENT_PRIMARY,
        EXAMPLE_STYLE_COLOR_ACCENT_SECONDARY, EXAMPLE_STYLE_COLOR_WARNING,
    };
    return _add_primitive(
        scene, panel, DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN, fan_positions, fan_color_roles, 8);
}



/*************************************************************************************************/
/*  Scenario callbacks                                                                           */
/*************************************************************************************************/

/**
 * Initialize the retained primitive visual scenario.
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

    return _add_primitives(ctx->scene, panel);
}



/**
 * Return the primitive visual scenario specification.
 *
 * @return scenario specification
 */
DvzScenarioSpec dvz_visual_primitive_scenario(void)
{
    return (DvzScenarioSpec){
        .id = "visual_primitive",
        .title = "visual_primitive",
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
 * Run the retained primitive visual example through the native scenario runner.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @return process exit code
 */
#ifndef DVZ_EXAMPLE_NO_MAIN
int main(int argc, char** argv)
{
    DvzScenarioSpec spec = dvz_visual_primitive_scenario();
    return dvz_scenario_run_native_cli(&spec, argc, argv) == 0 ? 0 : 1;
}
#endif
