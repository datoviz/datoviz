/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* update_partial - point visual with one retained data-range update.
 *
 * Scenario: feature.update_partial
 * Style: features, graphite_cyan, 1280x720 window target
 *
 * Build:  just example-c features/update_partial
 * Run:    ./build/examples/c/features/update_partial --live
 * Smoke:  ./build/examples/c/features/update_partial --png
 * Video:  ./build/examples/c/features/update_partial --offscreen-record 120
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

#define WIDTH  EXAMPLE_WINDOW_WIDTH
#define HEIGHT EXAMPLE_WINDOW_HEIGHT
#define POINT_COUNT 6u



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct UpdatePartialState
{
    DvzVisual* point;
    bool updated;
} UpdatePartialState;



/*************************************************************************************************/
/*  Forward declarations                                                                         */
/*************************************************************************************************/

DvzScenarioSpec dvz_example_update_partial_scenario(void);



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Upload the initial point arrays.
 *
 * @param visual point visual
 * @return true when the upload succeeds
 */
static bool _upload_initial_points(DvzVisual* visual)
{
    if (visual == NULL)
        return false;

    vec3 positions[POINT_COUNT] = {
        {-0.72f, -0.30f, 0.0f}, {-0.44f, -0.30f, 0.0f}, {-0.16f, -0.30f, 0.0f},
        {+0.16f, -0.30f, 0.0f}, {+0.44f, -0.30f, 0.0f}, {+0.72f, -0.30f, 0.0f},
    };
    DvzColor colors[POINT_COUNT] = {
        dvz_color_rgba(201, 209, 217, 210),
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ACCENT_PRIMARY),
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ACCENT_SECONDARY),
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ACCENT_SECONDARY),
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ACCENT_PRIMARY),
        dvz_color_rgba(201, 209, 217, 210),
    };
    float diameters[POINT_COUNT] = {32.0f, 36.0f, 42.0f, 42.0f, 36.0f, 32.0f};

    DvzVisualDataUpdate updates[] = {
        {.attr_name = "position", .data = positions, .item_count = POINT_COUNT},
        {.attr_name = "color", .data = colors, .item_count = POINT_COUNT},
        {.attr_name = "diameter_px", .data = diameters, .item_count = POINT_COUNT},
    };
    return dvz_visual_set_data_many(visual, updates, 3) == 0;
}



/**
 * Move the middle two points with a retained data-range update.
 *
 * @param visual point visual
 * @return true when the update succeeds
 */
static bool _upload_partial_positions(DvzVisual* visual)
{
    if (visual == NULL)
        return false;

    vec3 moved_positions[2] = {
        {-0.16f, +0.34f, 0.0f},
        {+0.16f, +0.34f, 0.0f},
    };

    return dvz_visual_set_data_range(visual, "position", 2, moved_positions, 2) == 0;
}



/*************************************************************************************************/
/*  Scenario callbacks                                                                           */
/*************************************************************************************************/

/**
 * Initialize the partial point-data update scenario.
 *
 * @param ctx scenario context
 * @param out_user scenario state output
 * @return true on success
 */
static bool _scenario_init(DvzScenarioContext* ctx, void** out_user)
{
    if (ctx == NULL || out_user == NULL)
        return false;

    UpdatePartialState* state = (UpdatePartialState*)dvz_calloc(1, sizeof(UpdatePartialState));
    if (state == NULL)
        return false;

    ctx->figure = dvz_figure(ctx->scene, ctx->width, ctx->height, 0);
    if (ctx->figure == NULL)
        goto error;

    DvzPanel* panel = dvz_panel_full(ctx->figure);
    if (panel == NULL)
        goto error;
    example_graphite_cyan_set_panel_background(panel);

    state->point = dvz_point(ctx->scene, 0);
    if (state->point == NULL)
        goto error;
    if (!_upload_initial_points(state->point))
        goto error;

    DvzPointStyleDesc style = dvz_point_style_desc();
    style.aspect = DVZ_SHAPE_ASPECT_FILLED;
    style.stroke_width_px = 0.0f;
    if (dvz_point_set_style(state->point, &style) != 0)
        goto error;

    if (dvz_visual_set_depth_test(state->point, false) != 0)
        goto error;

    if (dvz_panel_add_visual(panel, state->point, NULL) != 0)
        goto error;

    *out_user = state;
    return true;

error:
    dvz_free(state);
    return false;
}



/**
 * Apply the retained range update once after one second of scenario time.
 *
 * @param ctx scenario context
 * @param user scenario state
 */
static void _scenario_frame(DvzScenarioContext* ctx, void* user)
{
    if (ctx == NULL || user == NULL)
        return;

    UpdatePartialState* state = (UpdatePartialState*)user;
    if (!state->updated && ctx->time >= 1.0)
        state->updated = _upload_partial_positions(state->point);
}



/**
 * Destroy the partial point-data update scenario.
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
 * Return the partial update scenario specification.
 *
 * @return scenario specification
 */
DvzScenarioSpec dvz_example_update_partial_scenario(void)
{
    return (DvzScenarioSpec){
        .id = "feature_update_partial",
        .title = "Partial Data Update",
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
 * Run a minimal partial point-data update example through the native scenario runner.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @return process exit code
 */
#ifndef DVZ_EXAMPLE_NO_MAIN
int main(int argc, char** argv)
{
    DvzScenarioSpec spec = dvz_example_update_partial_scenario();
    return dvz_scenario_run_native_cli(&spec, argc, argv) == 0 ? 0 : 1;
}
#endif
