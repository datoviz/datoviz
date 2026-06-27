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
#include <string.h>

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
#define DVZ_ANIM_TIMER_DESC_KNOWN_FLAGS 0u
#define DVZ_ANIM_PHASE_DESC_KNOWN_FLAGS 0u
#define DVZ_TRACK_DESC_KNOWN_FLAGS 0u
#define DVZ_MOTION_DESC_KNOWN_FLAGS 0u



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef enum
{
    DVZ_TRACK_KIND_NONE = 0,
    DVZ_TRACK_KIND_CONSTANT,
    DVZ_TRACK_KIND_LINEAR,
    DVZ_TRACK_KIND_KEYFRAMES,
    DVZ_TRACK_KIND_CIRCLE2,
    DVZ_TRACK_KIND_CIRCLE3,
    DVZ_TRACK_KIND_ROTATION,
} DvzTrackKind;


struct DvzTrack
{
    DvzTrackKind kind;
    DvzTrackType type;
    DvzTrackRepeat repeat;
    DvzTrackTopology topology;
    DvzTrackInterpolation interpolation;
    double duration;
    float value[4];
    float start[4];
    float end[4];
    vec3 center;
    vec3 normal;
    vec3 basis_u;
    vec3 basis_v;
    float radius;
    float phase;
    float speed_rad_per_sec;
    vec3 axis;
    uint32_t count;
    double* times;
    float* values;
    float tension;
};



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


static bool _anim_timer_desc_validate(const DvzAnimTimerDesc* desc)
{
    if (desc == NULL)
    {
        log_error("timer animation descriptor is required");
        return false;
    }
    if (!DVZ_STRUCT_VALID(desc, DvzAnimTimerDesc, DVZ_ANIM_TIMER_DESC_KNOWN_FLAGS))
    {
        log_error("invalid DvzAnimTimerDesc ABI prologue");
        return false;
    }
    return true;
}



static uint32_t _track_type_components(DvzTrackType type)
{
    switch (type)
    {
    case DVZ_TRACK_FLOAT:
        return 1;
    case DVZ_TRACK_VEC2:
        return 2;
    case DVZ_TRACK_VEC3:
        return 3;
    case DVZ_TRACK_VEC4:
    case DVZ_TRACK_QUAT:
        return 4;
    default:
        return 0;
    }
}



static size_t _track_type_size(DvzTrackType type)
{
    return (size_t)_track_type_components(type) * sizeof(float);
}



static bool _track_type_valid(DvzTrackType type)
{
    return _track_type_components(type) > 0;
}



static bool _track_copy_value(DvzTrackType type, const void* src, float out[4])
{
    if (!_track_type_valid(type) || src == NULL || out == NULL)
        return false;
    out[0] = out[1] = out[2] = out[3] = 0.0f;
    memcpy(out, src, _track_type_size(type));
    return true;
}



static bool _track_desc_valid(
    const void* desc, size_t struct_size, uint32_t flags, const char* label)
{
    if (desc == NULL)
    {
        log_error("%s descriptor is required", label);
        return false;
    }
    const uint32_t* prologue = (const uint32_t*)desc;
    if (prologue[0] != struct_size || (prologue[1] & ~flags) != 0)
    {
        log_error("invalid %s ABI prologue", label);
        return false;
    }
    return true;
}



static double _track_wrap_time(double t, double start, double end, DvzTrackRepeat repeat)
{
    double duration = end - start;
    if (duration <= 0.0)
        return start;
    if (repeat == DVZ_TRACK_REPEAT_LOOP)
    {
        double wrapped = fmod(t - start, duration);
        if (wrapped < 0.0)
            wrapped += duration;
        return start + wrapped;
    }
    if (repeat == DVZ_TRACK_REPEAT_PINGPONG)
    {
        double width = 2.0 * duration;
        double wrapped = fmod(t - start, width);
        if (wrapped < 0.0)
            wrapped += width;
        if (wrapped > duration)
            wrapped = width - wrapped;
        return start + wrapped;
    }
    if (t < start)
        return start;
    if (t > end)
        return end;
    return t;
}



static bool _track_alloc(DvzTrack** out)
{
    ANN(out);
    DvzTrack* track = (DvzTrack*)dvz_calloc(1, sizeof(DvzTrack));
    if (track == NULL)
        return false;
    *out = track;
    return true;
}



static bool _track_make_circle3_basis(vec3 normal, vec3 u, vec3 v)
{
    if (glm_vec3_norm(normal) <= 0.0f)
        return false;
    vec3 n = {0};
    glm_vec3_normalize_to(normal, n);
    vec3 ref = {0.0f, 0.0f, 1.0f};
    if (fabsf(glm_vec3_dot(n, ref)) > 0.95f)
        glm_vec3_copy((vec3){1.0f, 0.0f, 0.0f}, ref);
    glm_vec3_cross(ref, n, u);
    if (glm_vec3_norm(u) <= 0.0f)
        return false;
    glm_vec3_normalize(u);
    glm_vec3_cross(n, u, v);
    glm_vec3_normalize(v);
    return true;
}



static void _track_lerp(const float* a, const float* b, uint32_t n, float u, float* out)
{
    for (uint32_t i = 0; i < n; i++)
        out[i] = a[i] + u * (b[i] - a[i]);
}



static void _track_catmull(
    const float* p0, const float* p1, const float* p2, const float* p3, uint32_t n, float u,
    float tension, float* out)
{
    float u2 = u * u;
    float u3 = u2 * u;
    float s = 0.5f * (1.0f - tension);
    for (uint32_t i = 0; i < n; i++)
    {
        float m1 = s * (p2[i] - p0[i]);
        float m2 = s * (p3[i] - p1[i]);
        out[i] = (2.0f * u3 - 3.0f * u2 + 1.0f) * p1[i] +
                 (u3 - 2.0f * u2 + u) * m1 + (-2.0f * u3 + 3.0f * u2) * p2[i] +
                 (u3 - u2) * m2;
    }
}



static const float* _track_catmull_neighbor(const DvzTrack* track, uint32_t k, int32_t offset)
{
    ANN(track);
    ANN(track->values);
    uint32_t n = _track_type_components(track->type);
    uint32_t i = 0;

    if (track->topology == DVZ_TRACK_TOPOLOGY_CLOSED && track->count > 3)
    {
        uint32_t unique_count = track->count - 1;
        i = (uint32_t)(((int32_t)k + offset + (int32_t)unique_count) % (int32_t)unique_count);
        return &track->values[i * n];
    }

    if (offset < 0)
        i = k > 0 ? k - 1 : k;
    else
        i = k + 2 < track->count ? k + 2 : k + 1;
    return &track->values[i * n];
}



static void _quat_slerp(const float* a, const float* b, float u, float* out)
{
    versor qa = {a[0], a[1], a[2], a[3]};
    versor qb = {b[0], b[1], b[2], b[3]};
    versor qr = GLM_QUAT_IDENTITY_INIT;
    glm_quat_normalize(qa);
    glm_quat_normalize(qb);
    glm_quat_slerp(qa, qb, u, qr);
    memcpy(out, qr, sizeof(versor));
}



static bool _controller_is_interacting(const DvzController* controller)
{
    if (controller == NULL || !controller->active)
        return false;
    switch (controller->type)
    {
    case DVZ_CONTROLLER_TYPE_PANZOOM:
        return controller->panzoom != NULL && controller->panzoom->interacting;
    case DVZ_CONTROLLER_TYPE_ARCBALL:
        return controller->arcball != NULL && dvz_arcball_is_interacting(controller->arcball);
    case DVZ_CONTROLLER_TYPE_FLY:
        return controller->fly != NULL && controller->fly->interacting;
    case DVZ_CONTROLLER_TYPE_TURNTABLE:
        return controller->turntable != NULL && controller->turntable->interacting;
    case DVZ_CONTROLLER_TYPE_ORBIT_CAMERA:
        return dvz_orbit_camera_is_interacting(controller->orbit_camera);
    default:
        return false;
    }
}



static bool _animation_interaction_allows_step(DvzAnimation* animation, double t)
{
    ANN(animation);
    if (animation->interaction_controller == NULL ||
        animation->interaction_policy == DVZ_ANIM_INTERACTION_CONTINUE)
        return true;

    bool interacting = _controller_is_interacting(animation->interaction_controller);
    if (interacting)
        animation->last_interaction_t = t;

    switch (animation->interaction_policy)
    {
    case DVZ_ANIM_INTERACTION_STOP:
        if (interacting)
        {
            animation->active = false;
            return false;
        }
        return true;
    case DVZ_ANIM_INTERACTION_PAUSE:
        return !interacting;
    case DVZ_ANIM_INTERACTION_RESUME_AFTER_IDLE:
        return !interacting && (t - animation->last_interaction_t) >= animation->interaction_idle_s;
    case DVZ_ANIM_INTERACTION_CONTINUE:
    default:
        return true;
    }
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
            scene->animations[i].speed = 1.0f;
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
    animation->speed = 1.0f;
    return animation;
}



/**
 * Return the interval tick due at a scene-clock time.
 *
 * @param animation timer animation
 * @param t current scene time
 * @return due tick index
 */
static uint64_t _animation_timer_due_tick(const DvzAnimation* animation, double t)
{
    ANN(animation);
    if (animation->period_s <= 0.0 || t <= animation->t_start)
        return 0;
    return (uint64_t)floor((t - animation->t_start) / animation->period_s);
}


/**
 * Fire one timer callback.
 *
 * @param animation timer animation
 * @param t current scene time
 * @param dt scene-clock delta
 * @param tick timer tick index
 */
static void _animation_timer_fire(DvzAnimation* animation, double t, double dt, uint64_t tick)
{
    ANN(animation);
    if (animation->timer_callback != NULL)
        animation->timer_callback(animation, t, dt, tick, animation->user_data);
}


/**
 * Advance a timer animation.
 *
 * @param animation timer animation
 * @param t current scene time
 * @param dt scene-clock delta
 */
static void _animation_timer_step(DvzAnimation* animation, double t, double dt)
{
    ANN(animation);
    if (!animation->active || animation->scene == NULL || animation->type != DVZ_ANIMATION_TIMER ||
        t < animation->t_start || animation->timer_callback == NULL)
        return;

    if (animation->timer_mode == DVZ_TIMER_EVERY_FRAME)
    {
        const uint64_t tick = animation->timer_tick++;
        _animation_timer_fire(animation, t, dt, tick);
        return;
    }

    if (animation->period_s <= 0.0)
        return;

    if (animation->timer_mode == DVZ_TIMER_INTERVAL)
    {
        const uint64_t due_tick = _animation_timer_due_tick(animation, t);
        if (due_tick < animation->timer_tick)
            return;
        _animation_timer_fire(animation, t, dt, due_tick);
        animation->timer_tick = due_tick + 1;
        animation->next_fire_t =
            animation->t_start + (double)animation->timer_tick * animation->period_s;
        return;
    }

    uint32_t emitted = 0;
    const uint32_t max_catch_up = animation->max_catch_up > 0 ? animation->max_catch_up : 1u;
    while (t + 1e-12 >= animation->next_fire_t && emitted < max_catch_up)
    {
        const uint64_t tick = animation->timer_tick++;
        _animation_timer_fire(animation, t, dt, tick);
        animation->next_fire_t =
            animation->t_start + (double)animation->timer_tick * animation->period_s;
        emitted++;
    }
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



static double _animation_local_time_step(DvzAnimation* animation, double t, double dt)
{
    ANN(animation);
    double step_dt = dt;
    if (t - step_dt < animation->t_start)
        step_dt = t - animation->t_start;
    if (step_dt < 0.0)
        step_dt = 0.0;
    animation->local_t += step_dt * (double)animation->speed;
    return animation->local_t;
}



static void _animation_track_step(DvzAnimation* animation, double t, double dt)
{
    ANN(animation);
    if (animation->track == NULL || animation->track_callback == NULL)
        return;
    float value[4] = {0};
    double local_t = _animation_local_time_step(animation, t, dt);
    if (!dvz_track_eval(animation->track, local_t, value))
        return;
    animation->track_callback(animation, local_t, value, animation->user_data);
}



static void _transform_motion_matrix(const DvzTransformMotionDesc* desc, double t, mat4 out)
{
    ANN(desc);
    ANN(out);
    glm_mat4_identity(out);

    vec3 translation = {0.0f, 0.0f, 0.0f};
    vec3 scale = {1.0f, 1.0f, 1.0f};
    versor rotation = GLM_QUAT_IDENTITY_INIT;
    if (desc->translation != NULL)
        (void)dvz_track_eval(desc->translation, t, translation);
    if (desc->scale != NULL)
        (void)dvz_track_eval(desc->scale, t, scale);
    if (desc->rotation != NULL)
        (void)dvz_track_eval(desc->rotation, t, rotation);

    mat4 translate = GLM_MAT4_IDENTITY_INIT;
    mat4 pivot = GLM_MAT4_IDENTITY_INIT;
    mat4 inv_pivot = GLM_MAT4_IDENTITY_INIT;
    mat4 rot = GLM_MAT4_IDENTITY_INIT;
    mat4 sc = GLM_MAT4_IDENTITY_INIT;
    mat4 tmp0 = GLM_MAT4_IDENTITY_INIT;
    mat4 tmp1 = GLM_MAT4_IDENTITY_INIT;
    mat4 tmp2 = GLM_MAT4_IDENTITY_INIT;

    glm_translate_make(translate, translation);
    vec3 pivot_vec = {0};
    glm_vec3_copy((vec3){desc->pivot[0], desc->pivot[1], desc->pivot[2]}, pivot_vec);
    glm_translate_make(pivot, pivot_vec);
    glm_translate_make(inv_pivot, (vec3){-desc->pivot[0], -desc->pivot[1], -desc->pivot[2]});
    glm_quat_mat4(rotation, rot);
    glm_scale_make(sc, scale);

    glm_mat4_mul(rot, sc, tmp0);
    glm_mat4_mul(pivot, tmp0, tmp1);
    glm_mat4_mul(tmp1, inv_pivot, tmp2);
    glm_mat4_mul(translate, tmp2, out);
}



static void _animation_visual_transform_step(DvzAnimation* animation, double t, double dt)
{
    ANN(animation);
    if (animation->visual == NULL)
        return;
    mat4 transform = GLM_MAT4_IDENTITY_INIT;
    double local_t = _animation_local_time_step(animation, t, dt);
    _transform_motion_matrix(&animation->transform_motion, local_t, transform);
    (void)dvz_visual_set_transform(animation->visual, transform);
}



static void _camera_world_up(vec3 eye, vec3 target, vec3 world_up, vec3 out)
{
    vec3 forward = {0};
    glm_vec3_sub(target, eye, forward);
    if (glm_vec3_norm(forward) <= 0.0f)
    {
        glm_vec3_copy(world_up, out);
        return;
    }
    glm_vec3_normalize(forward);
    float dot = glm_vec3_dot(world_up, forward);
    vec3 projected = {0};
    glm_vec3_scale(forward, dot, projected);
    glm_vec3_sub(world_up, projected, out);
    if (glm_vec3_norm(out) <= 1e-6f)
    {
        vec3 ref = {1.0f, 0.0f, 0.0f};
        if (fabsf(glm_vec3_dot(ref, forward)) > 0.95f)
            glm_vec3_copy((vec3){0.0f, 1.0f, 0.0f}, ref);
        glm_vec3_cross(forward, ref, out);
    }
    glm_vec3_normalize(out);
}



static void _animation_camera_motion_step(DvzAnimation* animation, double t, double dt)
{
    ANN(animation);
    if (animation->camera == NULL || animation->camera_motion.eye == NULL ||
        animation->camera_motion.target == NULL)
        return;

    double local_t = _animation_local_time_step(animation, t, dt);
    vec3 eye = {0}, target = {0}, up = {0};
    if (!dvz_track_eval(animation->camera_motion.eye, local_t, eye))
        return;
    if (!dvz_track_eval(animation->camera_motion.target, local_t, target))
        return;

    switch (animation->camera_motion.up_mode)
    {
    case DVZ_CAMERA_UP_TRACK:
        if (animation->camera_motion.up_track == NULL ||
            !dvz_track_eval(animation->camera_motion.up_track, local_t, up))
            glm_vec3_copy((vec3){0.0f, 1.0f, 0.0f}, up);
        break;
    case DVZ_CAMERA_UP_WORLD:
        _camera_world_up(eye, target, animation->camera_motion.up, up);
        break;
    case DVZ_CAMERA_UP_FIXED:
    default:
        glm_vec3_copy(animation->camera_motion.up, up);
        break;
    }
    dvz_camera_set_view(animation->camera, eye, target, up);
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Return a default timer animation descriptor.
 */
DvzAnimTimerDesc dvz_anim_timer_desc(void)
{
    return (DvzAnimTimerDesc){
        DVZ_STRUCT_INIT_FIELDS(DvzAnimTimerDesc),
        .mode = DVZ_TIMER_EVERY_FRAME,
        .max_catch_up = 4,
    };
}



/**
 * Return a default phase animation descriptor.
 */
DvzAnimPhaseDesc dvz_anim_phase_desc(void)
{
    return (DvzAnimPhaseDesc){
        DVZ_STRUCT_INIT_FIELDS(DvzAnimPhaseDesc),
    };
}



DvzTrackConstantDesc dvz_track_constant_desc(void)
{
    return (DvzTrackConstantDesc){DVZ_STRUCT_INIT_FIELDS(DvzTrackConstantDesc)};
}



DvzTrackLinearDesc dvz_track_linear_desc(void)
{
    return (DvzTrackLinearDesc){
        DVZ_STRUCT_INIT_FIELDS(DvzTrackLinearDesc),
        .duration = 1.0,
    };
}



DvzTrackKeyframesDesc dvz_track_keyframes_desc(void)
{
    return (DvzTrackKeyframesDesc){
        DVZ_STRUCT_INIT_FIELDS(DvzTrackKeyframesDesc),
        .interpolation = DVZ_TRACK_INTERP_LINEAR,
    };
}



DvzTrackCircle2Desc dvz_track_circle2_desc(void)
{
    return (DvzTrackCircle2Desc){
        DVZ_STRUCT_INIT_FIELDS(DvzTrackCircle2Desc),
        .radius = 1.0f,
    };
}



DvzTrackCircle3Desc dvz_track_circle3_desc(void)
{
    return (DvzTrackCircle3Desc){
        DVZ_STRUCT_INIT_FIELDS(DvzTrackCircle3Desc),
        .normal = {0.0f, 0.0f, 1.0f},
        .radius = 1.0f,
    };
}



DvzTrackRotationDesc dvz_track_rotation_desc(void)
{
    return (DvzTrackRotationDesc){
        DVZ_STRUCT_INIT_FIELDS(DvzTrackRotationDesc),
        .axis = {0.0f, 0.0f, 1.0f},
    };
}



DvzTransformMotionDesc dvz_transform_motion_desc(void)
{
    return (DvzTransformMotionDesc){
        DVZ_STRUCT_INIT_FIELDS(DvzTransformMotionDesc),
        .order = DVZ_TRANSFORM_ORDER_TRS,
    };
}



DvzCameraMotionDesc dvz_camera_motion_desc(void)
{
    return (DvzCameraMotionDesc){
        DVZ_STRUCT_INIT_FIELDS(DvzCameraMotionDesc),
        .up_mode = DVZ_CAMERA_UP_WORLD,
        .up = {0.0f, 1.0f, 0.0f},
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



DvzTrack* dvz_track_constant(const DvzTrackConstantDesc* desc)
{
    if (!_track_desc_valid(
            desc, sizeof(DvzTrackConstantDesc), DVZ_TRACK_DESC_KNOWN_FLAGS,
            "DvzTrackConstantDesc"))
        return NULL;
    if (!_track_type_valid(desc->type) || desc->value == NULL)
    {
        log_error("constant track requires a valid type and value");
        return NULL;
    }
    DvzTrack* track = NULL;
    if (!_track_alloc(&track))
        return NULL;
    track->kind = DVZ_TRACK_KIND_CONSTANT;
    track->type = desc->type;
    if (!_track_copy_value(desc->type, desc->value, track->value))
    {
        dvz_track_destroy(track);
        return NULL;
    }
    return track;
}



DvzTrack* dvz_track_linear(const DvzTrackLinearDesc* desc)
{
    if (!_track_desc_valid(
            desc, sizeof(DvzTrackLinearDesc), DVZ_TRACK_DESC_KNOWN_FLAGS, "DvzTrackLinearDesc"))
        return NULL;
    if (!_track_type_valid(desc->type) || desc->start == NULL || desc->end == NULL ||
        desc->duration <= 0.0)
    {
        log_error("linear track requires valid type, endpoints, and positive duration");
        return NULL;
    }
    DvzTrack* track = NULL;
    if (!_track_alloc(&track))
        return NULL;
    track->kind = DVZ_TRACK_KIND_LINEAR;
    track->type = desc->type;
    track->repeat = desc->repeat;
    track->duration = desc->duration;
    if (!_track_copy_value(desc->type, desc->start, track->start) ||
        !_track_copy_value(desc->type, desc->end, track->end))
    {
        dvz_track_destroy(track);
        return NULL;
    }
    return track;
}



DvzTrack* dvz_track_keyframes(const DvzTrackKeyframesDesc* desc)
{
    if (!_track_desc_valid(
            desc, sizeof(DvzTrackKeyframesDesc), DVZ_TRACK_DESC_KNOWN_FLAGS,
            "DvzTrackKeyframesDesc"))
        return NULL;
    if (!_track_type_valid(desc->type) || desc->count == 0 || desc->times == NULL ||
        desc->values == NULL)
    {
        log_error("keyframe track requires valid type, times, and values");
        return NULL;
    }
    if (desc->interpolation == DVZ_TRACK_INTERP_SLERP && desc->type != DVZ_TRACK_QUAT)
    {
        log_error("SLERP interpolation requires quaternion keyframes");
        return NULL;
    }
    if (desc->interpolation == DVZ_TRACK_INTERP_CUBIC_HERMITE ||
        desc->interpolation == DVZ_TRACK_INTERP_MONOTONE_CUBIC)
    {
        log_error("requested keyframe interpolation is not implemented yet");
        return NULL;
    }
    for (uint32_t i = 1; i < desc->count; i++)
    {
        if (desc->times[i] <= desc->times[i - 1])
        {
            log_error("keyframe times must be strictly increasing");
            return NULL;
        }
    }

    DvzTrack* track = NULL;
    if (!_track_alloc(&track))
        return NULL;
    track->kind = DVZ_TRACK_KIND_KEYFRAMES;
    track->type = desc->type;
    track->repeat = desc->repeat;
    track->topology = desc->topology;
    track->interpolation = desc->interpolation;
    track->count = desc->count;
    track->tension = desc->tension;
    track->times = (double*)dvz_calloc(desc->count, sizeof(double));
    track->values = (float*)dvz_calloc(
        (DvzSize)desc->count * _track_type_components(desc->type), sizeof(float));
    if (track->times == NULL || track->values == NULL)
    {
        dvz_track_destroy(track);
        return NULL;
    }
    memcpy(track->times, desc->times, (size_t)desc->count * sizeof(double));
    memcpy(
        track->values, desc->values,
        (size_t)desc->count * _track_type_components(desc->type) * sizeof(float));
    return track;
}



DvzTrack* dvz_track_circle2(const DvzTrackCircle2Desc* desc)
{
    if (!_track_desc_valid(
            desc, sizeof(DvzTrackCircle2Desc), DVZ_TRACK_DESC_KNOWN_FLAGS,
            "DvzTrackCircle2Desc"))
        return NULL;
    if (desc->radius <= 0.0f)
    {
        log_error("circle2 track radius must be positive");
        return NULL;
    }
    DvzTrack* track = NULL;
    if (!_track_alloc(&track))
        return NULL;
    track->kind = DVZ_TRACK_KIND_CIRCLE2;
    track->type = DVZ_TRACK_VEC2;
    track->center[0] = desc->center[0];
    track->center[1] = desc->center[1];
    track->radius = desc->radius;
    track->phase = desc->phase;
    track->speed_rad_per_sec = desc->speed_rad_per_sec;
    return track;
}



DvzTrack* dvz_track_circle3(const DvzTrackCircle3Desc* desc)
{
    if (!_track_desc_valid(
            desc, sizeof(DvzTrackCircle3Desc), DVZ_TRACK_DESC_KNOWN_FLAGS,
            "DvzTrackCircle3Desc"))
        return NULL;
    vec3 normal = {desc->normal[0], desc->normal[1], desc->normal[2]};
    if (desc->radius <= 0.0f || glm_vec3_norm(normal) <= 0.0f)
    {
        log_error("circle3 track requires positive radius and nonzero normal");
        return NULL;
    }
    DvzTrack* track = NULL;
    if (!_track_alloc(&track))
        return NULL;
    track->kind = DVZ_TRACK_KIND_CIRCLE3;
    track->type = DVZ_TRACK_VEC3;
    glm_vec3_copy((vec3){desc->center[0], desc->center[1], desc->center[2]}, track->center);
    glm_vec3_normalize_to(normal, track->normal);
    if (!_track_make_circle3_basis(track->normal, track->basis_u, track->basis_v))
    {
        dvz_track_destroy(track);
        return NULL;
    }
    track->radius = desc->radius;
    track->phase = desc->phase;
    track->speed_rad_per_sec = desc->speed_rad_per_sec;
    return track;
}



DvzTrack* dvz_track_rotation(const DvzTrackRotationDesc* desc)
{
    if (!_track_desc_valid(
            desc, sizeof(DvzTrackRotationDesc), DVZ_TRACK_DESC_KNOWN_FLAGS,
            "DvzTrackRotationDesc"))
        return NULL;
    vec3 axis = {desc->axis[0], desc->axis[1], desc->axis[2]};
    if (glm_vec3_norm(axis) <= 0.0f)
    {
        log_error("rotation track axis must be nonzero");
        return NULL;
    }
    DvzTrack* track = NULL;
    if (!_track_alloc(&track))
        return NULL;
    track->kind = DVZ_TRACK_KIND_ROTATION;
    track->type = DVZ_TRACK_QUAT;
    glm_vec3_normalize_to(axis, track->axis);
    track->phase = desc->phase;
    track->speed_rad_per_sec = desc->speed_rad_per_sec;
    return track;
}



bool dvz_track_eval(const DvzTrack* track, double t, void* out)
{
    if (track == NULL || out == NULL)
        return false;
    float* value = (float*)out;
    uint32_t n = _track_type_components(track->type);
    if (n == 0)
        return false;

    switch (track->kind)
    {
    case DVZ_TRACK_KIND_CONSTANT:
        memcpy(value, track->value, _track_type_size(track->type));
        return true;

    case DVZ_TRACK_KIND_LINEAR:
    {
        double local = _track_wrap_time(t, 0.0, track->duration, track->repeat);
        float u = track->duration > 0.0 ? (float)(local / track->duration) : 0.0f;
        _track_lerp(track->start, track->end, n, u, value);
        return true;
    }

    case DVZ_TRACK_KIND_KEYFRAMES:
    {
        if (track->count == 0)
            return false;
        if (track->count == 1)
        {
            memcpy(value, track->values, _track_type_size(track->type));
            return true;
        }
        double local =
            _track_wrap_time(t, track->times[0], track->times[track->count - 1], track->repeat);
        uint32_t k = 0;
        while (k + 1 < track->count && local > track->times[k + 1])
            k++;
        if (k + 1 >= track->count)
            k = track->count - 2;
        double dt = track->times[k + 1] - track->times[k];
        float u = dt > 0.0 ? (float)((local - track->times[k]) / dt) : 0.0f;
        const float* p1 = &track->values[k * n];
        const float* p2 = &track->values[(k + 1) * n];
        if (track->interpolation == DVZ_TRACK_INTERP_STEP)
            memcpy(value, p1, _track_type_size(track->type));
        else if (track->interpolation == DVZ_TRACK_INTERP_CATMULL_ROM)
        {
            const float* p0 = _track_catmull_neighbor(track, k, -1);
            const float* p3 = _track_catmull_neighbor(track, k, +2);
            _track_catmull(p0, p1, p2, p3, n, u, track->tension, value);
        }
        else if (track->interpolation == DVZ_TRACK_INTERP_SLERP && track->type == DVZ_TRACK_QUAT)
            _quat_slerp(p1, p2, u, value);
        else
            _track_lerp(p1, p2, n, u, value);
        return true;
    }

    case DVZ_TRACK_KIND_CIRCLE2:
    {
        float angle = track->phase + track->speed_rad_per_sec * (float)t;
        value[0] = track->center[0] + track->radius * cosf(angle);
        value[1] = track->center[1] + track->radius * sinf(angle);
        return true;
    }

    case DVZ_TRACK_KIND_CIRCLE3:
    {
        float angle = track->phase + track->speed_rad_per_sec * (float)t;
        float c = cosf(angle);
        float s = sinf(angle);
        for (uint32_t i = 0; i < 3; i++)
            value[i] = track->center[i] +
                       track->radius * (c * track->basis_u[i] + s * track->basis_v[i]);
        return true;
    }

    case DVZ_TRACK_KIND_ROTATION:
    {
        versor q = GLM_QUAT_IDENTITY_INIT;
        vec3 axis = {track->axis[0], track->axis[1], track->axis[2]};
        glm_quatv(q, track->phase + track->speed_rad_per_sec * (float)t, axis);
        memcpy(value, q, sizeof(versor));
        return true;
    }

    default:
        return false;
    }
}



void dvz_track_destroy(DvzTrack* track)
{
    if (track == NULL)
        return;
    dvz_free(track->times);
    dvz_free(track->values);
    dvz_free(track);
}



/**
 * Create a timer animation driven by the scene clock.
 *
 * @param scene owning scene
 * @param desc timer animation descriptor
 * @return the animation handle, or NULL on failure
 */
DvzAnimation* dvz_anim_timer(DvzScene* scene, const DvzAnimTimerDesc* desc)
{
    ANN(scene);
    if (!_anim_timer_desc_validate(desc))
        return NULL;
    if (desc->callback == NULL)
    {
        log_error("timer animation callback is required");
        return NULL;
    }
    if (desc->mode != DVZ_TIMER_EVERY_FRAME && desc->mode != DVZ_TIMER_INTERVAL &&
        desc->mode != DVZ_TIMER_CATCH_UP)
    {
        log_error("timer animation mode is invalid");
        return NULL;
    }
    if (desc->mode == DVZ_TIMER_EVERY_FRAME)
    {
        if (!isfinite(desc->period_s) || desc->period_s < 0.0)
        {
            log_error("every-frame timer animation period must be non-negative");
            return NULL;
        }
    }
    else if (!isfinite(desc->period_s) || desc->period_s <= 0.0)
    {
        log_error("interval timer animation period must be positive");
        return NULL;
    }
    DvzAnimation* animation = _animation_alloc(scene);
    if (animation == NULL)
        return NULL;
    animation->type = DVZ_ANIMATION_TIMER;
    animation->timer_mode = desc->mode;
    animation->period_s = desc->period_s;
    animation->max_catch_up = desc->max_catch_up;
    animation->timer_callback = desc->callback;
    animation->user_data = desc->user_data;
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



DvzAnimation* dvz_anim_track(
    DvzScene* scene, const DvzTrack* track, DvzTrackApplyCallback callback, void* user_data)
{
    ANN(scene);
    if (track == NULL || callback == NULL)
    {
        log_error("track animation requires a track and callback");
        return NULL;
    }
    DvzAnimation* animation = _animation_alloc(scene);
    if (animation == NULL)
        return NULL;
    animation->type = DVZ_ANIMATION_TRACK;
    animation->track = track;
    animation->track_callback = callback;
    animation->user_data = user_data;
    return animation;
}



DvzAnimation* dvz_anim_visual_transform(
    DvzScene* scene, DvzVisual* visual, const DvzTransformMotionDesc* desc)
{
    ANN(scene);
    ANN(visual);
    DvzTransformMotionDesc default_desc = dvz_transform_motion_desc();
    if (desc == NULL)
        desc = &default_desc;
    if (!_track_desc_valid(
            desc, sizeof(DvzTransformMotionDesc), DVZ_MOTION_DESC_KNOWN_FLAGS,
            "DvzTransformMotionDesc"))
        return NULL;
    if (desc->translation != NULL && desc->translation->type != DVZ_TRACK_VEC3)
    {
        log_error("transform translation track must be vec3");
        return NULL;
    }
    if (desc->rotation != NULL && desc->rotation->type != DVZ_TRACK_QUAT)
    {
        log_error("transform rotation track must be quat");
        return NULL;
    }
    if (desc->scale != NULL && desc->scale->type != DVZ_TRACK_VEC3)
    {
        log_error("transform scale track must be vec3");
        return NULL;
    }
    DvzAnimation* animation = _animation_alloc(scene);
    if (animation == NULL)
        return NULL;
    animation->type = DVZ_ANIMATION_VISUAL_TRANSFORM;
    animation->visual = visual;
    animation->transform_motion = *desc;
    return animation;
}



DvzAnimation*
dvz_anim_camera_motion(DvzScene* scene, DvzCamera* camera, const DvzCameraMotionDesc* desc)
{
    ANN(scene);
    ANN(camera);
    DvzCameraMotionDesc default_desc = dvz_camera_motion_desc();
    if (desc == NULL)
        desc = &default_desc;
    if (!_track_desc_valid(
            desc, sizeof(DvzCameraMotionDesc), DVZ_MOTION_DESC_KNOWN_FLAGS,
            "DvzCameraMotionDesc"))
        return NULL;
    if (desc->eye == NULL || desc->target == NULL || desc->eye->type != DVZ_TRACK_VEC3 ||
        desc->target->type != DVZ_TRACK_VEC3)
    {
        log_error("camera motion requires vec3 eye and target tracks");
        return NULL;
    }
    if (desc->up_mode == DVZ_CAMERA_UP_TRACK &&
        (desc->up_track == NULL || desc->up_track->type != DVZ_TRACK_VEC3))
    {
        log_error("camera up track must be vec3");
        return NULL;
    }
    vec3 up = {desc->up[0], desc->up[1], desc->up[2]};
    if (desc->up_mode != DVZ_CAMERA_UP_TRACK && glm_vec3_norm(up) <= 0.0f)
    {
        log_error("camera up vector must be nonzero");
        return NULL;
    }
    DvzAnimation* animation = _animation_alloc(scene);
    if (animation == NULL)
        return NULL;
    animation->type = DVZ_ANIMATION_CAMERA_MOTION;
    animation->camera = camera;
    animation->camera_motion = *desc;
    return animation;
}



void dvz_anim_set_interaction_policy(
    DvzAnimation* animation, DvzController* controller, DvzAnimInteractionPolicy policy,
    double idle_s)
{
    ANN(animation);
    animation->interaction_controller = controller;
    animation->interaction_policy = controller == NULL ? DVZ_ANIM_INTERACTION_CONTINUE : policy;
    animation->interaction_idle_s = idle_s > 0.0 ? idle_s : 0.0;
    animation->last_interaction_t = animation->scene != NULL ? animation->scene->clock.t : 0.0;
}



/**
 * Set the scalar speed used by phase animations, or the local-time multiplier used by track-backed
 * animations.
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
    if (animation->type == DVZ_ANIMATION_PHASE)
        animation->phase_speed = speed;
    else if (
        animation->type == DVZ_ANIMATION_TRACK ||
        animation->type == DVZ_ANIMATION_VISUAL_TRANSFORM ||
        animation->type == DVZ_ANIMATION_CAMERA_MOTION)
        animation->speed = speed;
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
    animation->local_t = 0.0;
    animation->timer_tick = 0;
    animation->next_fire_t = start;
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
            _animation_timer_step(animation, t, dt);
            break;

        case DVZ_ANIMATION_PHASE:
            if (_animation_should_advance(animation, t) &&
                _animation_interaction_allows_step(animation, t))
                _animation_phase_step(animation, dt);
            break;

        case DVZ_ANIMATION_TRACK:
            if (_animation_should_advance(animation, t) &&
                _animation_interaction_allows_step(animation, t))
                _animation_track_step(animation, t, dt);
            break;

        case DVZ_ANIMATION_VISUAL_TRANSFORM:
            if (_animation_should_advance(animation, t) &&
                _animation_interaction_allows_step(animation, t))
                _animation_visual_transform_step(animation, t, dt);
            break;

        case DVZ_ANIMATION_CAMERA_MOTION:
            if (_animation_should_advance(animation, t) &&
                _animation_interaction_allows_step(animation, t))
                _animation_camera_motion_step(animation, t, dt);
            break;

        default:
            break;
        }
    }
    _dvz_scene_controller_links_propagate(scene);
}
