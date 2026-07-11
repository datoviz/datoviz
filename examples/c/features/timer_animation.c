/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* This example updates point attributes every frame from scenario time.
 *
 * What to look for: the same retained point visual receives new position, color, and diameter_px
 * arrays on each frame. The eight points slide along a sine wave, pulse in size, and cycle through
 * palette colors without recreating the visual. This is useful for simulations, live instruments,
 * and time-dependent analyses where the data values change but the visualization object remains
 * the same.
 *
 * Scenario: features_timer_animation
 * Style: features, graphite_cyan, 1280x720 window target
 *
 * Build:  just example-c features/timer_animation
 * Run:    ./build/examples/c/features/timer_animation --live
 * Smoke:  ./build/examples/c/features/timer_animation --png
 * Video:  ./build/examples/c/features/timer_animation --offscreen-record 120
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
/*  Forward declarations                                                                         */
/*************************************************************************************************/

DvzScenarioSpec dvz_example_timer_animation_scenario(void);



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH       EXAMPLE_WINDOW_WIDTH
#define HEIGHT      EXAMPLE_WINDOW_HEIGHT
#define POINT_COUNT 8u

static const float TAU = 6.28318530718f;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct TimerAnimationState
{
    DvzVisual* point;
    vec3 positions[POINT_COUNT];
    DvzColor colors[POINT_COUNT];
    float diameters[POINT_COUNT];
} TimerAnimationState;



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Upload all animated point attributes.
 *
 * @param state animation state
 * @return true on success
 */
static bool _upload_timer_points(TimerAnimationState* state)
{
    if (state == NULL || state->point == NULL)
        return false;

    DvzVisualDataUpdate updates[] = {
        {.attr_name = "position", .data = state->positions, .item_count = POINT_COUNT},
        {.attr_name = "color", .data = state->colors, .item_count = POINT_COUNT},
        {.attr_name = "diameter_px", .data = state->diameters, .item_count = POINT_COUNT},
    };
    return dvz_visual_set_data_many(state->point, updates, 3) == 0;
}



/**
 * Fill the deterministic frame state at one scenario time.
 *
 * @param state animation state
 * @param t scenario time in seconds
 */
static void _fill_timer_points(TimerAnimationState* state, double t)
{
    if (state == NULL)
        return;

    const ExampleStyleColorRole palette[] = {
        EXAMPLE_STYLE_COLOR_ACCENT_PRIMARY,
        EXAMPLE_STYLE_COLOR_ACCENT_SECONDARY,
        EXAMPLE_STYLE_COLOR_WARNING,
        EXAMPLE_STYLE_COLOR_TEXT,
    };

    for (uint32_t i = 0; i < POINT_COUNT; i++)
    {
        const float u = (float)i / (float)(POINT_COUNT - 1u);
        const float phase = TAU * (u + 0.20f * (float)t);
        state->positions[i][0] = -0.78f + 1.56f * u;
        state->positions[i][1] = 0.22f * sinf(phase);
        state->positions[i][2] = 0.0f;
        state->diameters[i] = 28.0f + 18.0f * (0.5f + 0.5f * cosf(phase));
        const uint32_t color_index = (i + (uint32_t)(2.0 * t)) % DVZ_ARRAY_COUNT(palette);
        state->colors[i] = example_graphite_cyan_color(palette[color_index]);
    }
}



/*************************************************************************************************/
/*  Scenario callbacks                                                                           */
/*************************************************************************************************/

/**
 * Initialize the timer-animation scenario.
 *
 * @param ctx scenario context
 * @param out_user scenario state output
 * @return true on success
 */
static bool _scenario_init(DvzScenarioContext* ctx, void** out_user)
{
    if (ctx == NULL || out_user == NULL)
        return false;

    TimerAnimationState* state = (TimerAnimationState*)dvz_calloc(1, sizeof(TimerAnimationState));
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
    _fill_timer_points(state, 0.0);
    if (!_upload_timer_points(state))
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
 * Advance the timer-animation scenario for one frame.
 *
 * @param ctx scenario context
 * @param user scenario state
 */
static void _scenario_frame(DvzScenarioContext* ctx, void* user)
{
    if (ctx == NULL || user == NULL)
        return;

    TimerAnimationState* state = (TimerAnimationState*)user;
    const double time = ctx->preview_mode ? dvz_scenario_preview_time(ctx) : ctx->time;
    _fill_timer_points(state, time);
    (void)_upload_timer_points(state);
}



/**
 * Destroy the timer-animation scenario.
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
 * Return the timer-animation scenario specification.
 *
 * @return scenario specification
 */
DvzScenarioSpec dvz_example_timer_animation_scenario(void)
{
    return (DvzScenarioSpec){
        .id = "features_timer_animation",
        .title = "Timer Animation",
        .width = WIDTH,
        .height = HEIGHT,
        .fps = 60.0,
        .requirements = DVZ_SCENARIO_REQ_POINT_VISUAL | DVZ_SCENARIO_REQ_FRAME_CALLBACKS,
        .init = _scenario_init,
        .frame = _scenario_frame,
        .destroy = _scenario_destroy,
    };
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Run the timer-animation feature example through the native scenario runner.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @return process exit code
 */
#ifndef DVZ_EXAMPLE_NO_MAIN
int main(int argc, char** argv)
{
    DvzScenarioSpec spec = dvz_example_timer_animation_scenario();
    return dvz_scenario_run_native_cli(&spec, argc, argv) == 0 ? 0 : 1;
}
#endif
