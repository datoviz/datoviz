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

#include <math.h>
#include <string.h>

#include "_assertions.h"
#include "_controllers.h"
#include "_scene.h"
#include "datoviz/scene.h"
#include "interaction/animation_internal.h"
#include "test_scene.h"
#include "testing.h"



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct TimerTestState TimerTestState;
typedef struct PhaseTestState PhaseTestState;
typedef struct TrackTestState TrackTestState;

struct TimerTestState
{
    uint32_t calls;
    double last_t;
    double last_dt;
    double sum_dt;
    uint64_t last_tick;
    uint64_t tick_sum;
};



struct PhaseTestState
{
    uint32_t calls;
    float last_value;
    float last_delta;
};



struct TrackTestState
{
    uint32_t calls;
    double last_t;
    float last_float;
    vec3 last_value;
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
 * @param tick timer tick index
 * @param user_data timer test state
 */
static void
_timer_test_callback(DvzAnimation* animation, double t, double dt, uint64_t tick, void* user_data)
{
    (void)animation;
    TimerTestState* state = (TimerTestState*)user_data;
    ANN(state);
    state->calls++;
    state->last_t = t;
    state->last_dt = dt;
    state->sum_dt += dt;
    state->last_tick = tick;
    state->tick_sum += tick;
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



static void _track_vec3_callback(
    DvzAnimation* animation, double t, const void* value, void* user_data)
{
    (void)animation;
    TrackTestState* state = (TrackTestState*)user_data;
    ANN(state);
    const float* v = (const float*)value;
    state->calls++;
    state->last_t = t;
    glm_vec3_copy((vec3){v[0], v[1], v[2]}, state->last_value);
}



static void _track_float_callback(
    DvzAnimation* animation, double t, const void* value, void* user_data)
{
    (void)animation;
    TrackTestState* state = (TrackTestState*)user_data;
    ANN(state);
    state->calls++;
    state->last_t = t;
    state->last_float = *(const float*)value;
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
    DvzAnimTimerDesc timer_desc = dvz_anim_timer_desc();
    timer_desc.callback = _timer_test_callback;
    timer_desc.user_data = &state;
    DvzAnimation* timer = dvz_anim_timer(scene, &timer_desc);
    ANN(timer);
    dvz_anim_start(timer, 0.0);

    _dvz_scene_animations_step(scene, 100);
    AT(state.calls == 1);
    AT(state.last_tick == 0);
    AC(state.last_t, 0.0, EPS);
    AC(state.last_dt, 0.0, EPS);

    _dvz_scene_animations_step(scene, 200);
    AT(state.calls == 2);
    AT(state.last_tick == 1);
    AC(state.last_t, 0.5, EPS);
    AC(state.last_dt, 0.5, EPS);

    _dvz_scene_animations_step(scene, 300);
    AT(state.calls == 3);
    AT(state.last_tick == 2);
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
    DvzAnimTimerDesc timer_desc = dvz_anim_timer_desc();
    timer_desc.mode = DVZ_TIMER_INTERVAL;
    timer_desc.period_s = 0.2;
    timer_desc.callback = _timer_test_callback;
    timer_desc.user_data = &state;
    DvzAnimation* timer = dvz_anim_timer(scene, &timer_desc);
    ANN(timer);
    dvz_anim_start(timer, 0.0);

    _dvz_scene_animations_step(scene, 0);
    AT(state.calls == 1);
    AT(state.last_tick == 0);
    _dvz_scene_animations_step(scene, 0);
    AT(state.calls == 1);
    _dvz_scene_animations_step(scene, 0);
    AT(state.calls == 2);
    AT(state.last_tick == 1);

    dvz_anim_stop(timer);
    _dvz_scene_animations_step(scene, 0);
    _dvz_scene_animations_step(scene, 0);
    AT(state.calls == 2);

    dvz_scene_destroy(scene);
    return 0;
}


/**
 * Ensure timer descriptors reject invalid ABI prologues and invalid intervals.
 *
 * @param suite test suite
 * @param item test item
 * @return 0 on success
 */
int test_scene_animation_timer_descriptor_abi(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    ANN(item);

    DvzScene* scene = dvz_scene();
    ANN(scene);

    DvzAnimTimerDesc desc = dvz_anim_timer_desc();
    desc.callback = _timer_test_callback;
    desc.struct_size = 0;
    AT_EXPECTED_ERROR_STRICT(suite, dvz_anim_timer(scene, &desc) == NULL);

    desc = dvz_anim_timer_desc();
    desc.callback = _timer_test_callback;
    desc.flags = 1;
    AT_EXPECTED_ERROR_STRICT(suite, dvz_anim_timer(scene, &desc) == NULL);

    desc = dvz_anim_timer_desc();
    desc.mode = DVZ_TIMER_INTERVAL;
    desc.callback = _timer_test_callback;
    AT_EXPECTED_ERROR_STRICT(suite, dvz_anim_timer(scene, &desc) == NULL);

    dvz_scene_destroy(scene);
    return 0;
}



/**
 * Ensure catch-up timers emit bounded scheduled ticks after a long frame.
 *
 * @param suite test suite
 * @param item test item
 * @return 0 on success
 */
int test_scene_animation_timer_catch_up(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    ANN(item);

    DvzScene* scene = dvz_scene();
    ANN(scene);
    dvz_scene_set_clock_mode(scene, DVZ_CLOCK_REALTIME);

    TimerTestState state = {0};
    DvzAnimTimerDesc timer_desc = dvz_anim_timer_desc();
    timer_desc.mode = DVZ_TIMER_CATCH_UP;
    timer_desc.period_s = 0.05;
    timer_desc.max_catch_up = 2;
    timer_desc.callback = _timer_test_callback;
    timer_desc.user_data = &state;
    DvzAnimation* timer = dvz_anim_timer(scene, &timer_desc);
    ANN(timer);
    dvz_anim_start(timer, 0.0);

    _dvz_scene_animations_step(scene, 1000000000ULL);
    AT(state.calls == 1);
    AT(state.last_tick == 0);

    _dvz_scene_animations_step(scene, 1100000000ULL);
    AT(state.calls == 3);
    AT(state.last_tick == 2);
    AT(state.tick_sum == 3);

    _dvz_scene_animations_step(scene, 1150000000ULL);
    AT(state.calls == 4);
    AT(state.last_tick == 3);

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
 * Ensure phase animation descriptors reject invalid ABI prologues.
 *
 * @param suite test suite
 * @param item test item
 * @return 0 on success
 */
int test_scene_animation_phase_descriptor_abi(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    ANN(item);

    DvzScene* scene = dvz_scene();
    ANN(scene);

    DvzAnimPhaseDesc desc = dvz_anim_phase_desc();
    desc.wrap_max = 1.0f;
    desc.callback = _phase_test_callback;
    desc.struct_size = 0;
    AT_EXPECTED_ERROR_STRICT(suite, dvz_anim_phase(scene, &desc) == NULL);

    desc = dvz_anim_phase_desc();
    desc.wrap_max = 1.0f;
    desc.callback = _phase_test_callback;
    desc.flags = 1;
    AT_EXPECTED_ERROR_STRICT(suite, dvz_anim_phase(scene, &desc) == NULL);

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
                   DVZ_STRUCT_INIT_FIELDS(DvzAnimPhaseDesc),
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
                   DVZ_STRUCT_INIT_FIELDS(DvzAnimPhaseDesc),
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
                   DVZ_STRUCT_INIT_FIELDS(DvzAnimPhaseDesc),
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
    DvzAnimTimerDesc timer_desc = dvz_anim_timer_desc();
    timer_desc.callback = _timer_test_callback;
    timer_desc.user_data = &state;

    DvzAnimation* first = dvz_anim_timer(scene, &timer_desc);
    ANN(first);
    AT(scene->animation_count == 1);
    dvz_anim_destroy(first);
    AT(scene->animation_count == 0);

    DvzAnimation* second = dvz_anim_timer(scene, &timer_desc);
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
    DvzAnimTimerDesc timer_desc = dvz_anim_timer_desc();
    timer_desc.callback = _timer_test_callback;
    timer_desc.user_data = &state;
    DvzAnimation* timer = dvz_anim_timer(scene, &timer_desc);
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
int test_scene_animation_tracks(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    ANN(item);

    DvzTrack* constant = dvz_track_constant(&(DvzTrackConstantDesc){
        DVZ_STRUCT_INIT_FIELDS(DvzTrackConstantDesc),
        .type = DVZ_TRACK_VEC3,
        .value = (float[3]){1.0f, 2.0f, 3.0f},
    });
    ANN(constant);
    vec3 v = {0};
    AT(dvz_track_eval(constant, 123.0, v));
    AC(v[0], 1.0f, 1e-6f);
    AC(v[1], 2.0f, 1e-6f);
    AC(v[2], 3.0f, 1e-6f);

    DvzTrack* linear = dvz_track_linear(&(DvzTrackLinearDesc){
        DVZ_STRUCT_INIT_FIELDS(DvzTrackLinearDesc),
        .type = DVZ_TRACK_FLOAT,
        .start = (float[1]){0.0f},
        .end = (float[1]){10.0f},
        .duration = 2.0,
    });
    ANN(linear);
    float f = 0.0f;
    AT(dvz_track_eval(linear, 1.0, &f));
    AC(f, 5.0f, 1e-6f);

    double times[] = {0.0, 1.0, 2.0};
    float values[] = {0.0f, 0.0f, 10.0f, 0.0f, 20.0f, 0.0f};
    DvzTrack* keyframes = dvz_track_keyframes(&(DvzTrackKeyframesDesc){
        DVZ_STRUCT_INIT_FIELDS(DvzTrackKeyframesDesc),
        .type = DVZ_TRACK_VEC2,
        .count = 3,
        .times = times,
        .values = values,
        .interpolation = DVZ_TRACK_INTERP_LINEAR,
    });
    ANN(keyframes);
    vec2 p = {0};
    AT(dvz_track_eval(keyframes, 1.5, p));
    AC(p[0], 15.0f, 1e-6f);
    AC(p[1], 0.0f, 1e-6f);

    double loop_times[] = {0.0, 1.0, 2.0, 3.0};
    float loop_values[] = {
        0.0f, 0.0f, //
        1.0f, 0.0f, //
        1.0f, 1.0f, //
        0.0f, 0.0f,
    };
    DvzTrack* closed_keyframes = dvz_track_keyframes(&(DvzTrackKeyframesDesc){
        DVZ_STRUCT_INIT_FIELDS(DvzTrackKeyframesDesc),
        .type = DVZ_TRACK_VEC2,
        .count = 4,
        .times = loop_times,
        .values = loop_values,
        .repeat = DVZ_TRACK_REPEAT_LOOP,
        .topology = DVZ_TRACK_TOPOLOGY_CLOSED,
        .interpolation = DVZ_TRACK_INTERP_CATMULL_ROM,
    });
    ANN(closed_keyframes);
    AT(dvz_track_eval(closed_keyframes, 0.5, p));
    AC(p[0], 0.4375f, 1e-6f);
    AC(p[1], -0.125f, 1e-6f);
    AT(dvz_track_eval(closed_keyframes, 2.5, p));
    AC(p[0], 0.4375f, 1e-6f);
    AC(p[1], 0.5625f, 1e-6f);

    DvzTrack* circle = dvz_track_circle3(&(DvzTrackCircle3Desc){
        DVZ_STRUCT_INIT_FIELDS(DvzTrackCircle3Desc),
        .center = {0.0f, 0.0f, 0.0f},
        .normal = {0.0f, 0.0f, 1.0f},
        .radius = 2.0f,
    });
    ANN(circle);
    AT(dvz_track_eval(circle, 0.0, v));
    AC(glm_vec3_norm(v), 2.0f, 1e-5f);

    DvzTrack* rotation = dvz_track_rotation(&(DvzTrackRotationDesc){
        DVZ_STRUCT_INIT_FIELDS(DvzTrackRotationDesc),
        .axis = {0.0f, 0.0f, 1.0f},
        .speed_rad_per_sec = (float)M_PI,
    });
    ANN(rotation);
    versor q = GLM_QUAT_IDENTITY_INIT;
    AT(dvz_track_eval(rotation, 1.0, q));
    AC(fabsf(q[2]), 1.0f, 1e-5f);

    dvz_track_destroy(rotation);
    dvz_track_destroy(circle);
    dvz_track_destroy(closed_keyframes);
    dvz_track_destroy(keyframes);
    dvz_track_destroy(linear);
    dvz_track_destroy(constant);
    return 0;
}


int test_scene_animation_visual_transform(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    ANN(item);

    DvzScene* scene = dvz_scene();
    ANN(scene);
    dvz_scene_set_clock_mode(scene, DVZ_CLOCK_OFFLINE);
    dvz_scene_set_fps(scene, 1.0);
    DvzVisual* visual = dvz_point(scene, 0);
    ANN(visual);

    DvzTrack* rotation = dvz_track_rotation(&(DvzTrackRotationDesc){
        DVZ_STRUCT_INIT_FIELDS(DvzTrackRotationDesc),
        .axis = {0.0f, 0.0f, 1.0f},
        .speed_rad_per_sec = (float)M_PI,
    });
    ANN(rotation);
    DvzAnimation* animation = dvz_anim_visual_transform(
        scene, visual,
        &(DvzTransformMotionDesc){
            DVZ_STRUCT_INIT_FIELDS(DvzTransformMotionDesc),
            .rotation = rotation,
            .order = DVZ_TRANSFORM_ORDER_TRS,
        });
    ANN(animation);
    dvz_anim_start(animation, 0.0);
    _dvz_scene_animations_step(scene, 0);
    _dvz_scene_animations_step(scene, 0);

    mat4 transform = GLM_MAT4_IDENTITY_INIT;
    mat4 identity = GLM_MAT4_IDENTITY_INIT;
    AT(dvz_visual_get_transform(visual, transform) == 0);
    AT(memcmp(transform, identity, sizeof(mat4)) != 0);

    dvz_track_destroy(rotation);
    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_animation_track_speed_continuity(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    ANN(item);

    DvzScene* scene = dvz_scene();
    ANN(scene);
    dvz_scene_set_clock_mode(scene, DVZ_CLOCK_OFFLINE);
    dvz_scene_set_fps(scene, 1.0);

    DvzTrack* track = dvz_track_linear(&(DvzTrackLinearDesc){
        DVZ_STRUCT_INIT_FIELDS(DvzTrackLinearDesc),
        .type = DVZ_TRACK_FLOAT,
        .start = (float[1]){0.0f},
        .end = (float[1]){10.0f},
        .duration = 10.0,
    });
    ANN(track);

    TrackTestState state = {0};
    DvzAnimation* animation = dvz_anim_track(scene, track, _track_float_callback, &state);
    ANN(animation);
    dvz_anim_start(animation, 0.0);

    _dvz_scene_animations_step(scene, 0);
    AC((float)state.last_t, 0.0f, 1e-6f);
    AC(state.last_float, 0.0f, 1e-6f);
    _dvz_scene_animations_step(scene, 0);
    AC((float)state.last_t, 1.0f, 1e-6f);
    AC(state.last_float, 1.0f, 1e-6f);

    dvz_anim_set_speed(animation, 0.5f);
    _dvz_scene_animations_step(scene, 0);
    AC((float)state.last_t, 1.5f, 1e-6f);
    AC(state.last_float, 1.5f, 1e-6f);

    dvz_track_destroy(track);
    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_animation_camera_motion(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    ANN(item);

    DvzScene* scene = dvz_scene();
    ANN(scene);
    dvz_scene_set_clock_mode(scene, DVZ_CLOCK_OFFLINE);
    dvz_scene_set_fps(scene, 1.0);
    DvzCamera* camera = dvz_camera_create(NULL);
    ANN(camera);

    DvzTrack* eye = dvz_track_circle3(&(DvzTrackCircle3Desc){
        DVZ_STRUCT_INIT_FIELDS(DvzTrackCircle3Desc),
        .center = {0.0f, 0.0f, 0.0f},
        .normal = {0.0f, 0.0f, 1.0f},
        .radius = 3.0f,
        .speed_rad_per_sec = 0.5f,
    });
    DvzTrack* target = dvz_track_constant(&(DvzTrackConstantDesc){
        DVZ_STRUCT_INIT_FIELDS(DvzTrackConstantDesc),
        .type = DVZ_TRACK_VEC3,
        .value = (float[3]){0.0f, 0.0f, 0.0f},
    });
    ANN(eye);
    ANN(target);

    DvzAnimation* animation = dvz_anim_camera_motion(
        scene, camera,
        &(DvzCameraMotionDesc){
            DVZ_STRUCT_INIT_FIELDS(DvzCameraMotionDesc),
            .eye = eye,
            .target = target,
            .up_mode = DVZ_CAMERA_UP_WORLD,
            .up = {0.0f, 0.0f, 1.0f},
        });
    ANN(animation);
    dvz_anim_start(animation, 0.0);
    _dvz_scene_animations_step(scene, 0);
    _dvz_scene_animations_step(scene, 0);

    DvzCameraView actual_view = {0};
    dvz_camera_get_view(camera, &actual_view);
    AC(glm_vec3_norm(actual_view.eye), 3.0f, 1e-5f);
    AC(actual_view.target[0], 0.0f, 1e-6f);
    AC(actual_view.target[1], 0.0f, 1e-6f);
    AC(actual_view.target[2], 0.0f, 1e-6f);
    AC(glm_vec3_norm(actual_view.up), 1.0f, 1e-5f);

    dvz_track_destroy(target);
    dvz_track_destroy(eye);
    dvz_camera_destroy(camera);
    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_animation_interaction_stop(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    ANN(item);

    DvzScene* scene = dvz_scene();
    ANN(scene);
    dvz_scene_set_clock_mode(scene, DVZ_CLOCK_OFFLINE);
    dvz_scene_set_fps(scene, 1.0);
    DvzController* controller = dvz_arcball(scene, NULL);
    ANN(controller);
    DvzArcball* arcball = dvz_controller_arcball(controller);
    ANN(arcball);

    DvzTrack* track = dvz_track_constant(&(DvzTrackConstantDesc){
        DVZ_STRUCT_INIT_FIELDS(DvzTrackConstantDesc),
        .type = DVZ_TRACK_VEC3,
        .value = (float[3]){1.0f, 2.0f, 3.0f},
    });
    ANN(track);
    TrackTestState state = {0};
    DvzAnimation* animation = dvz_anim_track(scene, track, _track_vec3_callback, &state);
    ANN(animation);
    dvz_anim_set_interaction_policy(
        animation, controller, DVZ_ANIM_INTERACTION_STOP, 0.0);
    dvz_anim_start(animation, 0.0);

    _dvz_scene_animations_step(scene, 0);
    AT(state.calls == 1);
    arcball->interacting = true;
    _dvz_scene_animations_step(scene, 0);
    AT(!dvz_scene_has_active_animations(scene));

    dvz_track_destroy(track);
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
    TST_CASE(test_scene_animation_timer_descriptor_abi);
    TST_CASE(test_scene_animation_timer_catch_up);
    TST_CASE(test_scene_animation_realtime_delta_clamp);
    TST_CASE(test_scene_animation_phase_descriptor_abi);
    TST_CASE(test_scene_animation_phase_linear);
    TST_CASE(test_scene_animation_phase_wrap_and_setters);
    TST_CASE(test_scene_animation_phase_stop_restart);
    TST_CASE(test_scene_animation_destroy_reuses_slot);
    TST_CASE(test_scene_animation_active_query);
    TST_CASE(test_scene_animation_tracks);
    TST_CASE(test_scene_animation_visual_transform);
    TST_CASE(test_scene_animation_track_speed_continuity);
    TST_CASE(test_scene_animation_camera_motion);
    TST_CASE(test_scene_animation_interaction_stop);

    return 0;
}
