/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene animation tests                                                                        */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <string.h>

#include "_assertions.h"
#include "../_controllers.h"
#include "../_scene.h"
#include "datoviz/scene.h"
#include "test_scene.h"
#include "testing.h"



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct TimerTestState TimerTestState;
typedef struct PhaseTestState PhaseTestState;

struct TimerTestState
{
    uint32_t calls;
    double last_t;
    double last_dt;
    double sum_dt;
};



struct PhaseTestState
{
    uint32_t calls;
    float last_value;
    float last_delta;
};



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Record one timer callback invocation.
 *
 * @param animation timer animation
 * @param t current scene-clock time
 * @param dt elapsed scene-clock time since the previous step
 * @param user_data timer test state
 */
static void _timer_test_callback(DvzAnimation* animation, double t, double dt, void* user_data)
{
    (void)animation;
    TimerTestState* state = (TimerTestState*)user_data;
    ANN(state);
    state->calls++;
    state->last_t = t;
    state->last_dt = dt;
    state->sum_dt += dt;
}



/**
 * Record one phase callback invocation.
 *
 * @param animation phase animation
 * @param value current wrapped phase value
 * @param delta unwrapped phase delta applied on this step
 * @param user_data phase test state
 */
static void _phase_test_callback(
    DvzAnimation* animation, float value, float delta, void* user_data)
{
    (void)animation;
    PhaseTestState* state = (PhaseTestState*)user_data;
    ANN(state);
    state->calls++;
    state->last_value = value;
    state->last_delta = delta;
}



/*************************************************************************************************/
/*  Tests                                                                                        */
/*************************************************************************************************/

/**
 * Ensure an offline every-frame timer advances with deterministic fixed steps.
 *
 * @param suite test suite
 * @param item test item
 * @return 0 on success
 */
int test_scene_animation_offline_timer_every_frame(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    ANN(item);

    DvzScene* scene = dvz_scene();
    ANN(scene);
    dvz_scene_set_clock_mode(scene, DVZ_CLOCK_OFFLINE);
    dvz_scene_set_fps(scene, 2.0);

    TimerTestState state = {0};
    DvzAnimation* timer = dvz_anim_timer(scene, 0.0, _timer_test_callback, &state);
    ANN(timer);
    dvz_anim_start(timer, 0.0);

    _dvz_scene_animations_step(scene, 100);
    AT(state.calls == 1);
    AC(state.last_t, 0.0, EPS);
    AC(state.last_dt, 0.0, EPS);

    _dvz_scene_animations_step(scene, 200);
    AT(state.calls == 2);
    AC(state.last_t, 0.5, EPS);
    AC(state.last_dt, 0.5, EPS);

    _dvz_scene_animations_step(scene, 300);
    AT(state.calls == 3);
    AC(dvz_scene_clock_time(scene), 1.0, EPS);
    AC(dvz_scene_clock_dt(scene), 0.5, EPS);

    dvz_scene_destroy(scene);
    return 0;
}



/**
 * Ensure a nonzero timer period throttles callback delivery and stop disables it.
 *
 * @param suite test suite
 * @param item test item
 * @return 0 on success
 */
int test_scene_animation_timer_period_and_stop(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    ANN(item);

    DvzScene* scene = dvz_scene();
    ANN(scene);
    dvz_scene_set_clock_mode(scene, DVZ_CLOCK_OFFLINE);
    dvz_scene_set_fps(scene, 10.0);

    TimerTestState state = {0};
    DvzAnimation* timer = dvz_anim_timer(scene, 0.2, _timer_test_callback, &state);
    ANN(timer);
    dvz_anim_start(timer, 0.0);

    _dvz_scene_animations_step(scene, 0);
    AT(state.calls == 1);
    _dvz_scene_animations_step(scene, 0);
    AT(state.calls == 1);
    _dvz_scene_animations_step(scene, 0);
    AT(state.calls == 2);

    dvz_anim_stop(timer);
    _dvz_scene_animations_step(scene, 0);
    _dvz_scene_animations_step(scene, 0);
    AT(state.calls == 2);

    dvz_scene_destroy(scene);
    return 0;
}



/**
 * Ensure realtime clock deltas are measured from wall time and clamped.
 *
 * @param suite test suite
 * @param item test item
 * @return 0 on success
 */
int test_scene_animation_realtime_delta_clamp(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    ANN(item);

    DvzScene* scene = dvz_scene();
    ANN(scene);
    dvz_scene_set_clock_mode(scene, DVZ_CLOCK_REALTIME);

    _dvz_scene_animations_step(scene, 1000000000ULL);
    AC(dvz_scene_clock_dt(scene), 0.0, EPS);
    _dvz_scene_animations_step(scene, 2000000000ULL);
    AC(dvz_scene_clock_dt(scene), 0.1, EPS);
    AC(dvz_scene_clock_time(scene), 0.1, EPS);

    dvz_scene_destroy(scene);
    return 0;
}



/**
 * Ensure a phase animation advances linearly on the scene clock.
 *
 * @param suite test suite
 * @param item test item
 * @return 0 on success
 */
int test_scene_animation_phase_linear(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    ANN(item);

    DvzScene* scene = dvz_scene();
    ANN(scene);
    dvz_scene_set_clock_mode(scene, DVZ_CLOCK_OFFLINE);
    dvz_scene_set_fps(scene, 2.0);

    PhaseTestState state = {0};
    DvzAnimation* phase = dvz_anim_phase(
        scene, &(DvzAnimPhaseDesc){
                   .initial = 0.0f,
                   .speed = 1.0f,
                   .wrap_min = 0.0f,
                   .wrap_max = 10.0f,
                   .callback = _phase_test_callback,
                   .user_data = &state,
               });
    ANN(phase);
    dvz_anim_start(phase, 0.0);

    _dvz_scene_animations_step(scene, 100);
    AT(state.calls == 1);
    AC((double)state.last_value, 0.0, EPS);
    AC((double)state.last_delta, 0.0, EPS);

    _dvz_scene_animations_step(scene, 200);
    AT(state.calls == 2);
    AC((double)state.last_value, 0.5, EPS);
    AC((double)state.last_delta, 0.5, EPS);

    _dvz_scene_animations_step(scene, 300);
    AT(state.calls == 3);
    AC((double)state.last_value, 1.0, EPS);
    AC((double)state.last_delta, 0.5, EPS);

    dvz_scene_destroy(scene);
    return 0;
}



/**
 * Ensure a phase animation wraps values and honors live value/speed setters.
 *
 * @param suite test suite
 * @param item test item
 * @return 0 on success
 */
int test_scene_animation_phase_wrap_and_setters(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    ANN(item);

    DvzScene* scene = dvz_scene();
    ANN(scene);
    dvz_scene_set_clock_mode(scene, DVZ_CLOCK_OFFLINE);
    dvz_scene_set_fps(scene, 2.0);

    PhaseTestState state = {0};
    DvzAnimation* phase = dvz_anim_phase(
        scene, &(DvzAnimPhaseDesc){
                   .initial = 0.9f,
                   .speed = 0.4f,
                   .wrap_min = 0.0f,
                   .wrap_max = 1.0f,
                   .callback = _phase_test_callback,
                   .user_data = &state,
               });
    ANN(phase);
    dvz_anim_start(phase, 0.0);

    _dvz_scene_animations_step(scene, 100);
    AC((double)state.last_value, 0.9, EPS);
    AC((double)state.last_delta, 0.0, EPS);

    _dvz_scene_animations_step(scene, 200);
    AC((double)state.last_value, 0.1, EPS);
    AC((double)state.last_delta, 0.2, EPS);

    dvz_anim_set_speed(phase, 0.8f);
    _dvz_scene_animations_step(scene, 300);
    AC((double)state.last_value, 0.5, EPS);
    AC((double)state.last_delta, 0.4, EPS);

    dvz_anim_phase_set_value(phase, 1.25f);
    _dvz_scene_animations_step(scene, 400);
    AC((double)state.last_value, 0.65, EPS);
    AC((double)state.last_delta, 0.4, EPS);

    dvz_scene_destroy(scene);
    return 0;
}



/**
 * Ensure stopping and restarting a phase animation preserves its current value.
 *
 * @param suite test suite
 * @param item test item
 * @return 0 on success
 */
int test_scene_animation_phase_stop_restart(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    ANN(item);

    DvzScene* scene = dvz_scene();
    ANN(scene);
    dvz_scene_set_clock_mode(scene, DVZ_CLOCK_OFFLINE);
    dvz_scene_set_fps(scene, 4.0);

    PhaseTestState state = {0};
    DvzAnimation* phase = dvz_anim_phase(
        scene, &(DvzAnimPhaseDesc){
                   .initial = 1.0f,
                   .speed = 2.0f,
                   .wrap_min = -10.0f,
                   .wrap_max = 10.0f,
                   .callback = _phase_test_callback,
                   .user_data = &state,
               });
    ANN(phase);
    dvz_anim_start(phase, 0.0);

    _dvz_scene_animations_step(scene, 100);
    _dvz_scene_animations_step(scene, 200);
    AT(state.calls == 2);
    AC((double)state.last_value, 1.5, EPS);

    dvz_anim_stop(phase);
    _dvz_scene_animations_step(scene, 300);
    AT(state.calls == 2);
    AC((double)state.last_value, 1.5, EPS);

    dvz_anim_start(phase, 0.0);
    _dvz_scene_animations_step(scene, 400);
    AT(state.calls == 3);
    AC((double)state.last_value, 2.0, EPS);

    dvz_scene_destroy(scene);
    return 0;
}



/**
 * Ensure destroying a timer frees a scene animation slot for reuse.
 *
 * @param suite test suite
 * @param item test item
 * @return 0 on success
 */
int test_scene_animation_destroy_reuses_slot(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    ANN(item);

    DvzScene* scene = dvz_scene();
    ANN(scene);
    TimerTestState state = {0};

    DvzAnimation* first = dvz_anim_timer(scene, 0.0, _timer_test_callback, &state);
    ANN(first);
    AT(scene->animation_count == 1);
    dvz_anim_destroy(first);
    AT(scene->animation_count == 0);

    DvzAnimation* second = dvz_anim_timer(scene, 0.0, _timer_test_callback, &state);
    ANN(second);
    AT(second == first);
    AT(scene->animation_count == 1);

    dvz_scene_destroy(scene);
    return 0;
}


/**
 * Ensure active-animation query follows timer start/stop/destroy state.
 *
 * @param suite test suite
 * @param item test item
 * @return 0 on success
 */
int test_scene_animation_active_query(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    ANN(item);

    DvzScene* scene = dvz_scene();
    ANN(scene);
    AT(!dvz_scene_has_active_animations(scene));

    TimerTestState state = {0};
    DvzAnimation* timer = dvz_anim_timer(scene, 0.0, _timer_test_callback, &state);
    ANN(timer);
    AT(!dvz_scene_has_active_animations(scene));

    dvz_anim_start(timer, 0.0);
    AT(dvz_scene_has_active_animations(scene));

    dvz_anim_stop(timer);
    AT(!dvz_scene_has_active_animations(scene));

    dvz_anim_start(timer, 0.0);
    AT(dvz_scene_has_active_animations(scene));
    dvz_anim_destroy(timer);
    AT(!dvz_scene_has_active_animations(scene));

    dvz_scene_destroy(scene);
    return 0;
}


/**
 * Ensure an arcball spin animation advances and pauses during interaction.
 *
 * @param suite test suite
 * @param item test item
 * @return 0 on success
 */
int test_scene_animation_arcball_spin(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    ANN(item);

    DvzScene* scene = dvz_scene();
    ANN(scene);
    dvz_scene_set_clock_mode(scene, DVZ_CLOCK_OFFLINE);
    dvz_scene_set_fps(scene, 10.0);

    DvzArcball* arcball = _dvz_arcball(800.0f, 600.0f, 0);
    ANN(arcball);

    mat4 before = GLM_MAT4_IDENTITY_INIT;
    dvz_arcball_model(arcball, before);

    DvzAnimation* spin = dvz_anim_arcball_spin(
        scene, arcball, (vec3){0.0f, 1.0f, 0.0f}, 1.0f,
        DVZ_ARCBALL_SPIN_FLAGS_PAUSE_ON_INTERACTION);
    ANN(spin);
    dvz_anim_start(spin, 0.0);

    _dvz_scene_animations_step(scene, 0);
    _dvz_scene_animations_step(scene, 0);
    mat4 after = GLM_MAT4_IDENTITY_INIT;
    dvz_arcball_model(arcball, after);
    AT(memcmp(before, after, sizeof(mat4)) != 0);

    mat4 paused_before = GLM_MAT4_IDENTITY_INIT;
    dvz_arcball_model(arcball, paused_before);
    arcball->interacting = true;
    _dvz_scene_animations_step(scene, 0);
    mat4 paused_after = GLM_MAT4_IDENTITY_INIT;
    dvz_arcball_model(arcball, paused_after);
    AT(memcmp(paused_before, paused_after, sizeof(mat4)) == 0);

    dvz_anim_stop(spin);
    AT(!dvz_scene_has_active_animations(scene));

    dvz_arcball_destroy(arcball);
    dvz_scene_destroy(scene);
    return 0;
}



/**
 * Register scene animation tests.
 *
 * @param suite test suite
 * @return 0 on success
 */
int test_scene_animation(TstSuite* suite)
{
    ANN(suite);
    const char* tags = "scene";

    TST_MODULE(suite, "scene");
    TST_GROUP("animation");

    TST_CASE(test_scene_animation_offline_timer_every_frame);
    TST_CASE(test_scene_animation_timer_period_and_stop);
    TST_CASE(test_scene_animation_realtime_delta_clamp);
    TST_CASE(test_scene_animation_phase_linear);
    TST_CASE(test_scene_animation_phase_wrap_and_setters);
    TST_CASE(test_scene_animation_phase_stop_restart);
    TST_CASE(test_scene_animation_destroy_reuses_slot);
    TST_CASE(test_scene_animation_active_query);
    TST_CASE(test_scene_animation_arcball_spin);

    return 0;
}
