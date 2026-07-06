/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* panel_background shows a custom panel background behind a foreground primitive.
 *
 * What to look for: the panel rectangle is explicitly inset inside the figure and receives a
 * linear-gradient background before a triangle-list primitive uploads position, color, and normal
 * arrays. Compare the gradient panel area with the surrounding figure space; styled backgrounds
 * are useful for separating dense scientific plots, dark-field images, or instrument overlays from
 * the rest of a figure.
 *
 * Scenario: feature.panel_background
 * Style: features, graphite_cyan, 1280x720 window target
 *
 * Build:  just example-c features/panel_background
 * Run:    ./build/examples/c/features/panel_background --live
 * Smoke:  ./build/examples/c/features/panel_background --png
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
/*  Forward declarations                                                                         */
/*************************************************************************************************/

DvzScenarioSpec dvz_example_panel_background_scenario(void);



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH  EXAMPLE_WINDOW_WIDTH
#define HEIGHT EXAMPLE_WINDOW_HEIGHT
#define VERTEX_COUNT 6u



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Add one compact foreground primitive over the fixed panel background.
 *
 * @param scene scene owning the visual
 * @param panel panel receiving the visual
 * @return true when the visual was added
 */
static bool _add_foreground(DvzScene* scene, DvzPanel* panel)
{
    ANN(scene);
    ANN(panel);

    const vec3 positions[VERTEX_COUNT] = {
        {-0.70f, -0.40f, 0.0f}, {-0.10f, -0.40f, 0.0f}, {-0.40f, +0.42f, 0.0f},
        {+0.10f, -0.40f, 0.0f}, {+0.70f, -0.40f, 0.0f}, {+0.40f, +0.42f, 0.0f},
    };
    DvzColor colors[VERTEX_COUNT] = {
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ACCENT_PRIMARY),
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ACCENT_SECONDARY),
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_WARNING),
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ACCENT_SECONDARY),
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ACCENT_PRIMARY),
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_TEXT),
    };
    vec3 normals[VERTEX_COUNT] = {{0}};
    for (uint32_t i = 0; i < VERTEX_COUNT; i++)
        normals[i][2] = 1.0f;

    DvzVisual* visual = dvz_primitive(scene, DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0);
    if (visual == NULL)
        return false;

    DvzVisualDataUpdate updates[] = {
        {.attr_name = "position", .data = positions, .item_count = VERTEX_COUNT},
        {.attr_name = "color", .data = colors, .item_count = VERTEX_COUNT},
        {.attr_name = "normal", .data = normals, .item_count = VERTEX_COUNT},
    };
    if (dvz_visual_set_data_many(visual, updates, 3) != 0)
        return false;
    if (dvz_visual_set_depth_test(visual, false) != 0)
        return false;

    DvzVisualAttachDesc attach = dvz_visual_attach_desc();
    attach.coord_space = DVZ_VISUAL_COORD_VIEW;
    return dvz_panel_add_visual(panel, visual, &attach) == 0;
}



/*************************************************************************************************/
/*  Scenario callbacks                                                                           */
/*************************************************************************************************/

/**
 * Initialize the fixed panel background scenario.
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
    if (dvz_panel_set_desc(
            panel,
            &(DvzPanelDesc){.x = 0.10f, .y = 0.12f, .width = 0.80f, .height = 0.76f}) !=
        DVZ_OK)
        return false;
    DvzPanelBackgroundDesc background = {DVZ_STRUCT_INIT_FIELDS(DvzPanelBackgroundDesc),
        .type = DVZ_PANEL_BACKGROUND_LINEAR_GRADIENT,
        .gradient = {
            .start = {0.0f, 0.0f},
            .end = {1.0f, 1.0f},
            .color0 = {0.010f, 0.030f, 0.065f, 1.0f},
            .color1 = {0.025f, 0.345f, 0.380f, 1.0f},
        }};
    if (dvz_panel_set_background(panel, &background) != DVZ_OK)
        return false;

    return _add_foreground(ctx->scene, panel);
}



/**
 * Return the panel-background scenario specification.
 *
 * @return scenario specification
 */
DvzScenarioSpec dvz_example_panel_background_scenario(void)
{
    return (DvzScenarioSpec){
        .id = "feature_panel_background",
        .title = "Panel Background",
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
 * Run the fixed panel background feature example through the native scenario runner.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @return process exit code
 */
#ifndef DVZ_EXAMPLE_NO_MAIN
int main(int argc, char** argv)
{
    DvzScenarioSpec spec = dvz_example_panel_background_scenario();
    return dvz_scenario_run_native_cli(&spec, argc, argv) == 0 ? 0 : 1;
}
#endif
