/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* timer_animation - scene timer callback updating retained point data.
 *
 * Scenario: feature.timer_animation
 * Style: features, graphite_cyan, 1600x1200 capture target
 *
 * Build:  just example-c features/timer_animation
 * Run:    ./build/examples/c/features/timer_animation
 * Smoke:  ./build/examples/c/features/timer_animation 1
 * PNG:    DVZ_CAPTURE=png ./build/examples/c/features/timer_animation 1
 */



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <math.h>
#include <stdbool.h>
#include <stdint.h>

#include "datoviz/app.h"
#include "datoviz/scene.h"
#include "example_common.h"
#include "example_style.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH       1600u
#define HEIGHT      1200u
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
    DvzVisualDataUpdate updates[] = {
        {.attr_name = "position", .data = state->positions, .item_count = POINT_COUNT},
        {.attr_name = "color", .data = state->colors, .item_count = POINT_COUNT},
        {.attr_name = "diameter", .data = state->diameters, .item_count = POINT_COUNT},
    };
    return dvz_visual_set_data_many(state->point, updates, 3) == 0;
}



/**
 * Fill the deterministic frame state at one scene time.
 *
 * @param state animation state
 * @param t scene time in seconds
 */
static void _fill_timer_points(TimerAnimationState* state, double t)
{
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



/**
 * Timer callback that mutates retained visual data for the next frame.
 *
 * @param animation animation handle
 * @param t scene time in seconds
 * @param dt scene time delta in seconds
 * @param user_data TimerAnimationState pointer
 */
static void _timer_callback(DvzAnimation* animation, double t, double dt, void* user_data)
{
    (void)animation;
    (void)dt;

    TimerAnimationState* state = (TimerAnimationState*)user_data;
    if (state == NULL || state->point == NULL)
        return;
    _fill_timer_points(state, t);
    (void)_upload_timer_points(state);
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Run the timer-animation feature example.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @return process exit code
 */
int main(int argc, char** argv)
{
    const uint32_t frame_count = example_frame_count_any(argc, argv);
    DvzAppCaptureConfig capture = dvz_app_capture_config_from_env("feature_timer_animation");

    int ret = 1;
    DvzScene* scene = NULL;
    DvzApp* app = NULL;
    DvzView* win = NULL;
    TimerAnimationState state = {0};

    scene = dvz_scene();
    EXAMPLE_CHECK(scene != NULL, "dvz_scene() failed");
    dvz_scene_set_clock_mode(scene, DVZ_CLOCK_OFFLINE);
    dvz_scene_set_fps(scene, 12.0);

    DvzFigure* figure = dvz_figure(scene, WIDTH, HEIGHT, 0);
    EXAMPLE_CHECK(figure != NULL, "dvz_figure() failed");

    DvzPanel* panel = dvz_panel_full(figure);
    EXAMPLE_CHECK(panel != NULL, "dvz_panel_full() failed");
    example_graphite_cyan_set_panel_background(panel);

    state.point = dvz_point(scene, 0);
    EXAMPLE_CHECK(state.point != NULL, "dvz_point() failed");
    _fill_timer_points(&state, 0.0);
    EXAMPLE_CHECK(_upload_timer_points(&state), "initial point upload failed");

    DvzPointStyleDesc style = dvz_point_style_desc();
    style.aspect = DVZ_SHAPE_ASPECT_FILLED;
    style.stroke_width = 0.0f;
    EXAMPLE_CHECK(dvz_point_set_style(state.point, &style) == 0, "dvz_point_set_style() failed");
    EXAMPLE_CHECK(
        dvz_visual_set_depth_test(state.point, false) == 0,
        "dvz_visual_set_depth_test() failed");
    EXAMPLE_CHECK(
        dvz_panel_add_visual(panel, state.point, NULL) == 0,
        "dvz_panel_add_visual() failed");

    DvzAnimation* timer = dvz_anim_timer(scene, 0.0, _timer_callback, &state);
    EXAMPLE_CHECK(timer != NULL, "dvz_anim_timer() failed");
    dvz_anim_start(timer, 0.0);

    app = dvz_app(scene);
    EXAMPLE_CHECK(app != NULL, "dvz_app() failed (no GPU or display?)");

    win = dvz_view_glfw(app, figure, WIDTH, HEIGHT, "timer_animation");
    EXAMPLE_CHECK(win != NULL, "dvz_view_glfw() failed (GLFW unavailable?)");

    EXAMPLE_CHECK(
        example_run_with_capture(app, win, frame_count, &capture),
        "example_run_with_capture() failed");
    ret = 0;

cleanup:
    if (app != NULL)
        dvz_app_destroy(app);
    if (scene != NULL)
        dvz_scene_destroy(scene);
    return ret;
}
