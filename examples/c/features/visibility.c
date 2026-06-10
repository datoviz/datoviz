/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* visibility - retained visual visibility toggled on a runner frame.
 *
 * Scenario: feature.visibility
 * Style: features, graphite_cyan, 1600x1200 capture target
 *
 * Build:  just example-c features/visibility
 * Run:    ./build/examples/c/features/visibility --live
 * Smoke:  ./build/examples/c/features/visibility --png
 * Video:  ./build/examples/c/features/visibility --offscreen-record 120
 */



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>

#include "_alloc.h"
#include "datoviz/scene.h"
#include "example_style.h"
#include "runner/scenario_runner.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH       1600u
#define HEIGHT      1200u
#define POINT_COUNT 1u



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct VisibilityState
{
    DvzVisual* hidden_point;
    bool visible;
} VisibilityState;



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Add one single-point visual.
 *
 * @param scene scene owning the visual
 * @param panel panel receiving the visual
 * @param x point X position
 * @param color point color
 * @param visible whether the visual should render
 * @param out_visual created visual output
 * @return true when the visual was added
 */
static bool _add_point_visual(
    DvzScene* scene, DvzPanel* panel, float x, DvzColor color, bool visible,
    DvzVisual** out_visual)
{
    const vec3 positions[POINT_COUNT] = {{x, 0.0f, 0.0f}};
    const DvzColor colors[POINT_COUNT] = {color};
    const float diameters[POINT_COUNT] = {72.0f};

    DvzVisual* point = dvz_point(scene, 0);
    if (point == NULL)
        return false;

    DvzVisualDataUpdate updates[] = {
        {.attr_name = "position", .data = positions, .item_count = POINT_COUNT},
        {.attr_name = "color", .data = colors, .item_count = POINT_COUNT},
        {.attr_name = "diameter", .data = diameters, .item_count = POINT_COUNT},
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

    dvz_visual_set_visible(point, visible);
    if (dvz_panel_add_visual(panel, point, NULL) != 0)
        return false;

    if (out_visual != NULL)
        *out_visual = point;
    return true;
}



/*************************************************************************************************/
/*  Scenario callbacks                                                                           */
/*************************************************************************************************/

/**
 * Initialize the retained visual visibility scenario.
 *
 * @param ctx scenario context
 * @param out_user scenario state output
 * @return true on success
 */
static bool _scenario_init(DvzScenarioContext* ctx, void** out_user)
{
    if (ctx == NULL || out_user == NULL)
        return false;

    VisibilityState* state = (VisibilityState*)dvz_calloc(1, sizeof(VisibilityState));
    if (state == NULL)
        return false;

    ctx->figure = dvz_figure(ctx->scene, ctx->width, ctx->height, 0);
    if (ctx->figure == NULL)
        goto error;

    DvzPanel* panel = dvz_panel_full(ctx->figure);
    if (panel == NULL)
        goto error;
    example_graphite_cyan_set_panel_background(panel);

    /*
     * This example uses separate visual objects only to expose visual-level visibility toggling.
     * Production scenes should batch related items in one visual when they share the same family.
     */
    if (!_add_point_visual(
            ctx->scene, panel, -0.42f,
            example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ACCENT_PRIMARY), true, NULL))
        goto error;
    if (!_add_point_visual(
            ctx->scene, panel, 0.0f, example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ERROR), false,
            &state->hidden_point))
        goto error;
    if (!_add_point_visual(
            ctx->scene, panel, +0.42f,
            example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ACCENT_SECONDARY), true, NULL))
        goto error;

    *out_user = state;
    return true;

error:
    dvz_free(state);
    return false;
}



/**
 * Toggle the middle point at 2 Hz.
 *
 * @param ctx scenario context
 * @param user scenario state
 */
static void _scenario_frame(DvzScenarioContext* ctx, void* user)
{
    if (ctx == NULL || user == NULL)
        return;

    VisibilityState* state = (VisibilityState*)user;
    const bool visible = ((uint32_t)(ctx->time * 2.0)) % 2u == 0u;
    if (visible != state->visible)
    {
        dvz_visual_set_visible(state->hidden_point, visible);
        state->visible = visible;
    }
}



/**
 * Destroy the retained visual visibility scenario.
 *
 * @param ctx scenario context
 * @param user scenario state
 */
static void _scenario_destroy(DvzScenarioContext* ctx, void* user)
{
    (void)ctx;
    dvz_free(user);
}



/**
 * Return the visibility scenario specification.
 *
 * @return scenario specification
 */
static DvzScenarioSpec _visibility_scenario(void)
{
    return (DvzScenarioSpec){
        .id = "feature_visibility",
        .title = "visibility",
        .width = WIDTH,
        .height = HEIGHT,
        .fps = 60.0,
        .init = _scenario_init,
        .frame = _scenario_frame,
        .destroy = _scenario_destroy,
    };
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Run the retained visual visibility feature example through the native scenario runner.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @return process exit code
 */
int main(int argc, char** argv)
{
    DvzScenarioSpec spec = _visibility_scenario();
    return dvz_scenario_run_native_cli(&spec, argc, argv) == 0 ? 0 : 1;
}
