/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene animation                                                                              */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <math.h>
#include <stdbool.h>
#include <stdint.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_log.h"
#include "_scene.h"
#include "animation_internal.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define DVZ_SCENE_DEFAULT_FPS 60.0
#define DVZ_SCENE_MAX_REALTIME_DT 0.1
#define DVZ_ANIM_PHASE_DESC_KNOWN_FLAGS 0u



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

static bool _anim_phase_desc_validate(const DvzAnimPhaseDesc* desc)
{
    if (desc == NULL)
        return false;
    if (!DVZ_STRUCT_VALID(desc, DvzAnimPhaseDesc, DVZ_ANIM_PHASE_DESC_KNOWN_FLAGS))
    {
        log_error("invalid DvzAnimPhaseDesc ABI prologue");
        return false;
    }
    return true;
}



/**
 * Allocate one scene-owned animation slot.
 *
 * @param scene owning scene
 * @return animation slot, or NULL when the scene capacity is exhausted
 */
static DvzAnimation* _animation_alloc(DvzScene* scene)
{
    ANN(scene);
    for (uint32_t i = 0; i < scene->animation_count; i++)
    {
        if (scene->animations[i].scene == NULL)
        {
            dvz_memset(&scene->animations[i], sizeof(DvzAnimation), 0, sizeof(DvzAnimation));
            scene->animations[i].scene = scene;
            return &scene->animations[i];
        }
    }
    if (scene->animation_count >= DVZ_SCENE_MAX_ANIMATIONS)
    {
        log_error("scene animation capacity exceeded");
        return NULL;
    }
    DvzAnimation* animation = &scene->animations[scene->animation_count++];
    dvz_memset(animation, sizeof(DvzAnimation), 0, sizeof(DvzAnimation));
    animation->scene = scene;
    return animation;
}



/**
 * Return whether a timer animation should fire at the current scene time.
 *
 * @param animation timer animation
 * @param t current scene time
 * @return true when the callback should be invoked
 */
static bool _animation_timer_should_fire(const DvzAnimation* animation, double t)
{
    ANN(animation);
    if (!animation->active || animation->scene == NULL || animation->type != DVZ_ANIMATION_TIMER)
        return false;
    if (t < animation->t_start)
        return false;
    if (animation->period_s <= 0.0)
        return true;
    return (t - animation->last_fire_t) >= animation->period_s;
}


/**
 * Return whether a non-timer animation should advance at the current scene time.
 *
 * @param animation animation handle
 * @param t current scene time
 * @return true when the animation should advance
 */
static bool _animation_should_advance(const DvzAnimation* animation, double t)
{
    ANN(animation);
    if (!animation->active || animation->scene == NULL)
        return false;
    return t >= animation->t_start;
}



/**
 * Wrap a phase value into a half-open interval.
 *
 * @param value input value
 * @param min minimum wrapped value
 * @param max maximum wrapped value
 * @return wrapped value
 */
static float _animation_wrap_phase(float value, float min, float max)
{
    float width = max - min;
    if (width <= 0.0f)
        return value;
    float wrapped = value - width * floorf((value - min) / width);
    if (wrapped >= max)
        wrapped = min;
    return wrapped;
}



/**
 * Advance a wrapped linear phase animation by one scene-clock delta.
 *
 * @param animation phase animation
 * @param dt elapsed scene-clock time since the previous step
 */
static void _animation_phase_step(DvzAnimation* animation, double dt)
{
    ANN(animation);
    if (animation->phase_callback == NULL)
        return;

    float delta = animation->phase_speed * (float)dt;
    animation->phase_value = _animation_wrap_phase(
        animation->phase_value + delta, animation->phase_wrap_min, animation->phase_wrap_max);
    animation->phase_callback(animation, animation->phase_value, delta, animation->user_data);
}



/**
 * Advance an arcball spin animation by one scene-clock delta.
 *
 * @param animation arcball spin animation
 * @param dt elapsed scene-clock time since the previous step
 */
static void _animation_arcball_spin_step(DvzAnimation* animation, double dt)
{
    ANN(animation);
    if (animation->arcball == NULL || dt == 0.0 || animation->speed_rad_per_sec == 0.0f)
        return;
    if ((animation->flags & DVZ_ARCBALL_SPIN_FLAGS_PAUSE_ON_INTERACTION) &&
        dvz_arcball_is_interacting(animation->arcball))
    {
        return;
    }
    dvz_arcball_rotate_axis(
        animation->arcball, animation->speed_rad_per_sec * (float)dt, animation->axis);
}



/**
 * Return the next scene-clock delta.
 *
 * @param scene target scene
 * @param wall_time_ns current wall-clock timestamp in nanoseconds
 * @return clock delta in seconds
 */
static double _scene_clock_next_dt(DvzScene* scene, uint64_t wall_time_ns)
{
    ANN(scene);
    DvzSceneClock* clock = &scene->clock;
    if (!clock->initialized)
    {
        clock->initialized = true;
        clock->last_wall_time_ns = wall_time_ns;
        return 0.0;
    }
    if (clock->mode == DVZ_CLOCK_OFFLINE)
        return 1.0 / clock->fps;
    uint64_t elapsed_ns = 0;
    if (wall_time_ns >= clock->last_wall_time_ns)
        elapsed_ns = wall_time_ns - clock->last_wall_time_ns;
    clock->last_wall_time_ns = wall_time_ns;
    double dt = (double)elapsed_ns / 1000000000.0;
    return dt > DVZ_SCENE_MAX_REALTIME_DT ? DVZ_SCENE_MAX_REALTIME_DT : dt;
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Return a default phase animation descriptor.
 */
DvzAnimPhaseDesc dvz_anim_phase_desc(void)
{
    return (DvzAnimPhaseDesc){
        DVZ_STRUCT_INIT_FIELDS(DvzAnimPhaseDesc),
    };
}



/**
 * Set the scene clock mode used by animations.
 *
 * @param scene target scene
 * @param mode realtime or offline clock mode
 */
void dvz_scene_set_clock_mode(DvzScene* scene, DvzSceneClockMode mode)
{
    ANN(scene);
    scene->clock.mode = mode;
    scene->clock.dt = 0.0;
    scene->clock.initialized = false;
}



/**
 * Set the scene clock frame rate used by offline mode and timer period resolution.
 *
 * @param scene target scene
 * @param fps frames per second, must be positive
 */
void dvz_scene_set_fps(DvzScene* scene, double fps)
{
    ANN(scene);
    if (fps <= 0.0)
    {
        log_error("scene clock fps must be positive");
        return;
    }
    scene->clock.fps = fps;
}



/**
 * Return the current scene clock time in seconds.
 *
 * @param scene target scene
 * @return current scene time
 */
double dvz_scene_clock_time(const DvzScene* scene)
{
    ANN(scene);
    return scene->clock.t;
}



/**
 * Return the last scene clock delta in seconds.
 *
 * @param scene target scene
 * @return last scene time delta
 */
double dvz_scene_clock_dt(const DvzScene* scene)
{
    ANN(scene);
    return scene->clock.dt;
}


/**
 * Return whether the scene has at least one active animation.
 *
 * @param scene target scene
 * @return true when an animation is active
 */
bool dvz_scene_has_active_animations(const DvzScene* scene)
{
    ANN(scene);
    for (uint32_t i = 0; i < scene->animation_count; i++)
    {
        const DvzAnimation* animation = &scene->animations[i];
        if (animation->scene == scene && animation->active)
            return true;
    }
    return false;
}



/**
 * Create a timer animation driven by the scene clock.
 *
 * @param scene owning scene
 * @param period_s callback period in seconds, or 0 for every scene frame
 * @param callback timer callback
 * @param user_data opaque pointer forwarded to the callback
 * @return the animation handle, or NULL on failure
 */
DvzAnimation* dvz_anim_timer(
    DvzScene* scene, double period_s, DvzAnimTimerCallback callback, void* user_data)
{
    ANN(scene);
    if (callback == NULL)
    {
        log_error("timer animation callback is required");
        return NULL;
    }
    if (period_s < 0.0)
    {
        log_error("timer animation period must be non-negative");
        return NULL;
    }
    DvzAnimation* animation = _animation_alloc(scene);
    if (animation == NULL)
        return NULL;
    animation->type = DVZ_ANIMATION_TIMER;
    animation->period_s = period_s;
    animation->timer_callback = callback;
    animation->user_data = user_data;
    animation->last_fire_t = -period_s;
    return animation;
}



/**
 * Create a wrapped linear phase animation driven by the scene clock.
 *
 * @param scene owning scene
 * @param desc phase animation descriptor
 * @return the animation handle, or NULL on failure
 */
DvzAnimation* dvz_anim_phase(DvzScene* scene, const DvzAnimPhaseDesc* desc)
{
    ANN(scene);
    if (!_anim_phase_desc_validate(desc))
        return NULL;
    if (desc->callback == NULL)
    {
        log_error("phase animation callback is required");
        return NULL;
    }
    if (!isfinite(desc->initial) || !isfinite(desc->speed) || !isfinite(desc->wrap_min) ||
        !isfinite(desc->wrap_max))
    {
        log_error("phase animation values must be finite");
        return NULL;
    }
    if (desc->wrap_max <= desc->wrap_min)
    {
        log_error("phase animation wrap range must be increasing");
        return NULL;
    }

    DvzAnimation* animation = _animation_alloc(scene);
    if (animation == NULL)
        return NULL;
    animation->type = DVZ_ANIMATION_PHASE;
    animation->phase_callback = desc->callback;
    animation->user_data = desc->user_data;
    animation->phase_value =
        _animation_wrap_phase(desc->initial, desc->wrap_min, desc->wrap_max);
    animation->phase_speed = desc->speed;
    animation->phase_wrap_min = desc->wrap_min;
    animation->phase_wrap_max = desc->wrap_max;
    return animation;
}



/**
 * Create an arcball spin animation driven by the scene clock.
 *
 * @param scene owning scene
 * @param arcball target arcball controller
 * @param axis rotation axis
 * @param speed_rad_per_sec angular speed in radians per second
 * @param flags DvzArcballSpinFlags bitmask
 * @return the animation handle, or NULL on failure
 */
DvzAnimation* dvz_anim_arcball_spin(
    DvzScene* scene, DvzArcball* arcball, vec3 axis, float speed_rad_per_sec, uint32_t flags)
{
    ANN(scene);
    ANN(arcball);
    if (glm_vec3_norm(axis) == 0.0f)
    {
        log_error("arcball spin axis must be nonzero");
        return NULL;
    }
    DvzAnimation* animation = _animation_alloc(scene);
    if (animation == NULL)
        return NULL;
    animation->type = DVZ_ANIMATION_ARCBALL_SPIN;
    animation->arcball = arcball;
    glm_vec3_normalize_to(axis, animation->axis);
    animation->speed_rad_per_sec = speed_rad_per_sec;
    animation->flags = flags;
    return animation;
}



/**
 * Set the scalar speed used by phase and arcball spin animations.
 *
 * @param animation animation handle
 * @param speed scalar speed in units per second
 */
void dvz_anim_set_speed(DvzAnimation* animation, float speed)
{
    ANN(animation);
    if (!isfinite(speed))
    {
        log_error("animation speed must be finite");
        return;
    }
    switch (animation->type)
    {
    case DVZ_ANIMATION_PHASE:
        animation->phase_speed = speed;
        break;
    case DVZ_ANIMATION_ARCBALL_SPIN:
        animation->speed_rad_per_sec = speed;
        break;
    default:
        break;
    }
}



/**
 * Set the current value of a phase animation.
 *
 * @param animation phase animation handle
 * @param value new phase value
 */
void dvz_anim_phase_set_value(DvzAnimation* animation, float value)
{
    ANN(animation);
    if (animation->type != DVZ_ANIMATION_PHASE)
        return;
    if (!isfinite(value))
    {
        log_error("phase animation value must be finite");
        return;
    }
    animation->phase_value =
        _animation_wrap_phase(value, animation->phase_wrap_min, animation->phase_wrap_max);
}



/**
 * Start or restart an animation at a scene-clock time.
 *
 * @param animation animation handle
 * @param t_start scene-clock start time, or 0 for immediate start
 */
void dvz_anim_start(DvzAnimation* animation, double t_start)
{
    ANN(animation);
    if (animation->scene == NULL)
        return;
    double start = t_start;
    if (start <= 0.0)
        start = animation->scene->clock.t;
    animation->t_start = start;
    animation->last_fire_t = animation->period_s > 0.0 ? start - animation->period_s : start;
    animation->active = true;
}



/**
 * Stop an animation while keeping the handle valid.
 *
 * @param animation animation handle
 */
void dvz_anim_stop(DvzAnimation* animation)
{
    ANN(animation);
    animation->active = false;
}



/**
 * Destroy an animation handle owned by its scene.
 *
 * @param animation animation handle
 */
void dvz_anim_destroy(DvzAnimation* animation)
{
    if (animation == NULL)
        return;
    DvzScene* scene = animation->scene;
    dvz_memset(animation, sizeof(DvzAnimation), 0, sizeof(DvzAnimation));
    if (scene == NULL)
        return;
    while (scene->animation_count > 0 &&
           scene->animations[scene->animation_count - 1].scene == NULL)
    {
        scene->animation_count--;
    }
}



/**
 * Advance the scene clock and run active animation callbacks.
 *
 * @param scene target scene
 * @param wall_time_ns current wall-clock timestamp in nanoseconds
 */
void _dvz_scene_animations_step(DvzScene* scene, uint64_t wall_time_ns)
{
    if (scene == NULL)
        return;
    if (scene->clock.fps <= 0.0)
        scene->clock.fps = DVZ_SCENE_DEFAULT_FPS;
    scene->clock.dt = _scene_clock_next_dt(scene, wall_time_ns);
    scene->clock.t += scene->clock.dt;
    double t = scene->clock.t;
    double dt = scene->clock.dt;
    uint32_t animation_count = scene->animation_count;
    for (uint32_t i = 0; i < animation_count; i++)
    {
        DvzAnimation* animation = &scene->animations[i];
        switch (animation->type)
        {
        case DVZ_ANIMATION_TIMER:
            if (!_animation_timer_should_fire(animation, t))
                continue;
            animation->last_fire_t = t;
            animation->timer_callback(animation, t, dt, animation->user_data);
            break;

        case DVZ_ANIMATION_PHASE:
            if (_animation_should_advance(animation, t))
                _animation_phase_step(animation, dt);
            break;

        case DVZ_ANIMATION_ARCBALL_SPIN:
            if (_animation_should_advance(animation, t))
                _animation_arcball_spin_step(animation, dt);
            break;

        default:
            break;
        }
    }
    _dvz_scene_controller_links_propagate(scene);
}
