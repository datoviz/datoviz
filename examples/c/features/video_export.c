/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* video_export - portable scenario using the native runner's live/capture modes.
 *
 * Scenario: feature.video_export
 * Style: features, graphite_cyan, 1600x1200 capture target
 *
 * Build:  just example-c features/video_export
 * Live:   ./build/examples/c/features/video_export --live
 * Video:  ./build/examples/c/features/video_export --live-record 120
 * Hidden: ./build/examples/c/features/video_export --offscreen-record 120
 */



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <math.h>
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
#define TICK_COUNT  9u
#define POINT_COUNT (TICK_COUNT + 1u)

static const float TAU = 6.28318530718f;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct VideoExportScenario
{
    DvzVisual* point;
    vec3 positions[POINT_COUNT];
    DvzColor colors[POINT_COUNT];
    float diameters[POINT_COUNT];
} VideoExportScenario;



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Upload the diagnostic point visual.
 *
 * @param state scenario state
 * @return true on success
 */
static bool _upload_points(VideoExportScenario* state)
{
    if (state == NULL || state->point == NULL)
        return false;

    DvzVisualDataUpdate updates[] = {
        {.attr_name = "position", .data = state->positions, .item_count = POINT_COUNT},
        {.attr_name = "color", .data = state->colors, .item_count = POINT_COUNT},
        {.attr_name = "diameter", .data = state->diameters, .item_count = POINT_COUNT},
    };
    return dvz_visual_set_data_many(state->point, updates, 3) == 0;
}



/**
 * Fill fixed tick marks and the moving diagnostic marker at one scenario time.
 *
 * @param state scenario state
 * @param t scenario time in seconds
 */
static void _fill_points(VideoExportScenario* state, double t)
{
    if (state == NULL)
        return;

    for (uint32_t i = 0; i < TICK_COUNT; i++)
    {
        const float u = TICK_COUNT > 1 ? (float)i / (float)(TICK_COUNT - 1u) : 0.0f;
        state->positions[i][0] = -0.80f + 1.60f * u;
        state->positions[i][1] = -0.38f;
        state->positions[i][2] = 0.0f;
        state->diameters[i] = i == TICK_COUNT / 2u ? 18.0f : 12.0f;
        state->colors[i] = example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_GRID);
    }

    const float phase = 0.12f * TAU * (float)t;
    const uint32_t cursor = POINT_COUNT - 1u;
    state->positions[cursor][0] = 0.78f * sinf(phase);
    state->positions[cursor][1] = 0.18f * cosf(0.65f * phase);
    state->positions[cursor][2] = 0.0f;
    state->diameters[cursor] = 58.0f + 10.0f * sinf(1.7f * phase);
    state->colors[cursor] = example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ACCENT_PRIMARY);
}



/*************************************************************************************************/
/*  Scenario callbacks                                                                           */
/*************************************************************************************************/

/**
 * Initialize the video-export scenario.
 *
 * @param ctx scenario context
 * @param out_user scenario state output
 * @return true on success
 */
static bool _scenario_init(DvzScenarioContext* ctx, void** out_user)
{
    if (ctx == NULL || out_user == NULL)
        return false;

    VideoExportScenario* state = (VideoExportScenario*)dvz_calloc(1, sizeof(VideoExportScenario));
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

    _fill_points(state, 0.0);
    if (!_upload_points(state))
        goto error;

    DvzPointStyleDesc style = dvz_point_style_desc();
    style.aspect = DVZ_SHAPE_ASPECT_FILLED;
    style.stroke_width = 0.0f;
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
 * Advance the video-export scenario for one runner frame.
 *
 * @param ctx scenario context
 * @param user scenario state
 */
static void _scenario_frame(DvzScenarioContext* ctx, void* user)
{
    if (ctx == NULL || user == NULL)
        return;

    VideoExportScenario* state = (VideoExportScenario*)user;
    _fill_points(state, ctx->time);
    (void)_upload_points(state);
}



/**
 * Destroy the video-export scenario.
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
 * Return the video-export scenario specification.
 *
 * @return scenario specification
 */
static DvzScenarioSpec _video_export_scenario(void)
{
    return (DvzScenarioSpec){
        .id = "feature_video_export",
        .title = "video_export",
        .width = WIDTH,
        .height = HEIGHT,
        .fps = 60.0,
        .requirements = DVZ_SCENARIO_REQ_POINT_VISUAL | DVZ_SCENARIO_REQ_FRAME_CALLBACKS |
                        DVZ_SCENARIO_REQ_NATIVE_CAPTURE,
        .init = _scenario_init,
        .frame = _scenario_frame,
        .destroy = _scenario_destroy,
    };
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Run the video-export feature example through the native scenario runner.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @return process exit code
 */
int main(int argc, char** argv)
{
    DvzScenarioSpec spec = _video_export_scenario();
    return dvz_scenario_run_native_cli(&spec, argc, argv) == 0 ? 0 : 1;
}
