/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* update_visual_data - retained point visual with full data replacement.
 *
 * Scenario: feature.update_visual_data
 * Style: features, graphite_cyan, 1600x1200 capture target
 *
 * Build:  just example-c features/update_visual_data
 * Run:    ./build/examples/c/features/update_visual_data --live
 * Smoke:  ./build/examples/c/features/update_visual_data --png
 * Video:  ./build/examples/c/features/update_visual_data --offscreen-record 120
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
#define POINT_COUNT 7u



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct UpdateVisualDataState
{
    DvzVisual* point;
    bool updated;
} UpdateVisualDataState;



/*************************************************************************************************/
/*  Forward declarations                                                                         */
/*************************************************************************************************/

DvzScenarioSpec dvz_example_update_visual_data_scenario(void);



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Upload either the initial or replacement point arrays to one retained point visual.
 *
 * @param visual point visual
 * @param updated whether to upload the replacement data
 * @return true when the upload succeeds
 */
static bool _upload_points(DvzVisual* visual, bool updated)
{
    if (visual == NULL)
        return false;

    const vec3 initial_positions[POINT_COUNT] = {
        {-0.72f, -0.32f, 0.0f}, {-0.48f, -0.32f, 0.0f}, {-0.24f, -0.32f, 0.0f},
        {+0.00f, -0.32f, 0.0f}, {+0.24f, -0.32f, 0.0f}, {+0.48f, -0.32f, 0.0f},
        {+0.72f, -0.32f, 0.0f},
    };
    const vec3 updated_positions[POINT_COUNT] = {
        {-0.72f, +0.18f, 0.0f}, {-0.48f, -0.02f, 0.0f}, {-0.24f, +0.30f, 0.0f},
        {+0.00f, +0.04f, 0.0f}, {+0.24f, +0.30f, 0.0f}, {+0.48f, -0.02f, 0.0f},
        {+0.72f, +0.18f, 0.0f},
    };
    DvzColor initial_colors[POINT_COUNT] = {
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_GRID),
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_GRID),
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_GRID),
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_GRID),
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_GRID),
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_GRID),
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_GRID),
    };
    DvzColor updated_colors[POINT_COUNT] = {
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ACCENT_PRIMARY),
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ACCENT_SECONDARY),
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_WARNING),
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_TEXT),
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_WARNING),
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ACCENT_SECONDARY),
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ACCENT_PRIMARY),
    };
    const float initial_diameters[POINT_COUNT] = {18.0f, 18.0f, 18.0f, 18.0f, 18.0f, 18.0f, 18.0f};
    const float updated_diameters[POINT_COUNT] = {26.0f, 34.0f, 44.0f, 58.0f, 44.0f, 34.0f, 26.0f};

    DvzVisualDataUpdate updates[] = {
        {
            .attr_name = "position",
            .data = updated ? updated_positions : initial_positions,
            .item_count = POINT_COUNT,
        },
        {
            .attr_name = "color",
            .data = updated ? updated_colors : initial_colors,
            .item_count = POINT_COUNT,
        },
        {
            .attr_name = "diameter_px",
            .data = updated ? updated_diameters : initial_diameters,
            .item_count = POINT_COUNT,
        },
    };
    return dvz_visual_set_data_many(visual, updates, 3) == 0;
}



/*************************************************************************************************/
/*  Scenario callbacks                                                                           */
/*************************************************************************************************/

/**
 * Initialize the retained visual data update scenario.
 *
 * @param ctx scenario context
 * @param out_user scenario state output
 * @return true on success
 */
static bool _scenario_init(DvzScenarioContext* ctx, void** out_user)
{
    if (ctx == NULL || out_user == NULL)
        return false;

    UpdateVisualDataState* state =
        (UpdateVisualDataState*)dvz_calloc(1, sizeof(UpdateVisualDataState));
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
    if (!_upload_points(state->point, false))
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
 * Replace retained point data once after one second of scenario time.
 *
 * @param ctx scenario context
 * @param user scenario state
 */
static void _scenario_frame(DvzScenarioContext* ctx, void* user)
{
    if (ctx == NULL || user == NULL)
        return;

    UpdateVisualDataState* state = (UpdateVisualDataState*)user;
    if (!state->updated && ctx->time >= 1.0)
    {
        state->updated = _upload_points(state->point, true);
    }
}



/**
 * Destroy the retained visual data update scenario.
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
 * Return the retained visual data update scenario specification.
 *
 * @return scenario specification
 */
DvzScenarioSpec dvz_example_update_visual_data_scenario(void)
{
    return (DvzScenarioSpec){
        .id = "feature_update_visual_data",
        .title = "update_visual_data",
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
 * Run the full retained visual data update feature example through the native scenario runner.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @return process exit code
 */
#ifndef DVZ_EXAMPLE_NO_MAIN
int main(int argc, char** argv)
{
    DvzScenarioSpec spec = dvz_example_update_visual_data_scenario();
    return dvz_scenario_run_native_cli(&spec, argc, argv) == 0 ? 0 : 1;
}
#endif
