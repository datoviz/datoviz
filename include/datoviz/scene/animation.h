/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene animation                                                                              */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "datoviz/common/macros.h"
#include "datoviz/math/types.h"
#include "datoviz/scene/types.h"



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/

typedef enum
{
    DVZ_CLOCK_REALTIME = 0,
    DVZ_CLOCK_OFFLINE,
} DvzSceneClockMode;


typedef enum
{
    DVZ_TRACK_FLOAT,
    DVZ_TRACK_VEC2,
    DVZ_TRACK_VEC3,
    DVZ_TRACK_VEC4,
    DVZ_TRACK_QUAT,
} DvzTrackType;


typedef enum
{
    DVZ_TRACK_REPEAT_NONE,
    DVZ_TRACK_REPEAT_LOOP,
    DVZ_TRACK_REPEAT_PINGPONG,
} DvzTrackRepeat;


typedef enum
{
    DVZ_TRACK_INTERP_STEP,
    DVZ_TRACK_INTERP_LINEAR,
    DVZ_TRACK_INTERP_CATMULL_ROM,
    DVZ_TRACK_INTERP_CUBIC_HERMITE,
    DVZ_TRACK_INTERP_SLERP,
    DVZ_TRACK_INTERP_MONOTONE_CUBIC,
} DvzTrackInterpolation;


typedef enum
{
    DVZ_TRACK_TANGENT_AUTO,
    DVZ_TRACK_TANGENT_FLAT,
    DVZ_TRACK_TANGENT_USER,
} DvzTrackTangentMode;


typedef enum
{
    DVZ_TRANSFORM_ORDER_TRS,
} DvzTransformOrder;


typedef enum
{
    DVZ_CAMERA_UP_FIXED,
    DVZ_CAMERA_UP_WORLD,
    DVZ_CAMERA_UP_TRACK,
} DvzCameraUpMode;


typedef enum
{
    DVZ_ANIM_INTERACTION_CONTINUE,
    DVZ_ANIM_INTERACTION_STOP,
    DVZ_ANIM_INTERACTION_PAUSE,
    DVZ_ANIM_INTERACTION_RESUME_AFTER_IDLE,
} DvzAnimInteractionPolicy;



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzAnimation DvzAnimation;
typedef struct DvzTrack DvzTrack;

typedef void (*DvzAnimTimerCallback)(
    DvzAnimation* animation, double t, double dt, void* user_data);

typedef void (*DvzAnimPhaseCallback)(
    DvzAnimation* animation, float value, float delta, void* user_data);

typedef void (*DvzTrackApplyCallback)(
    DvzAnimation* animation, double t, const void* value, void* user_data);



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct DvzAnimPhaseDesc DvzAnimPhaseDesc;
typedef struct DvzTrackConstantDesc DvzTrackConstantDesc;
typedef struct DvzTrackLinearDesc DvzTrackLinearDesc;
typedef struct DvzTrackKeyframesDesc DvzTrackKeyframesDesc;
typedef struct DvzTrackCircle2Desc DvzTrackCircle2Desc;
typedef struct DvzTrackCircle3Desc DvzTrackCircle3Desc;
typedef struct DvzTrackRotationDesc DvzTrackRotationDesc;
typedef struct DvzTransformMotionDesc DvzTransformMotionDesc;
typedef struct DvzCameraMotionDesc DvzCameraMotionDesc;

struct DvzAnimPhaseDesc
{
    uint32_t struct_size;
    uint32_t flags;
    float initial;
    float speed;
    float wrap_min;
    float wrap_max;
    DvzAnimPhaseCallback callback;
    void* user_data;
};


struct DvzTrackConstantDesc
{
    uint32_t struct_size;
    uint32_t flags;
    DvzTrackType type;
    const void* value;
};


struct DvzTrackLinearDesc
{
    uint32_t struct_size;
    uint32_t flags;
    DvzTrackType type;
    const void* start;
    const void* end;
    double duration;
    DvzTrackRepeat repeat;
};


struct DvzTrackKeyframesDesc
{
    uint32_t struct_size;
    uint32_t flags;
    DvzTrackType type;
    uint32_t count;
    const double* times;
    const void* values;
    DvzTrackRepeat repeat;
    DvzTrackInterpolation interpolation;
    DvzTrackTangentMode tangents;
    float tension;
    const void* in_tangents;
    const void* out_tangents;
};


struct DvzTrackCircle2Desc
{
    uint32_t struct_size;
    uint32_t flags;
    vec2 center;
    float radius;
    float phase;
    float speed_rad_per_sec;
};


struct DvzTrackCircle3Desc
{
    uint32_t struct_size;
    uint32_t flags;
    vec3 center;
    vec3 normal;
    float radius;
    float phase;
    float speed_rad_per_sec;
};


struct DvzTrackRotationDesc
{
    uint32_t struct_size;
    uint32_t flags;
    vec3 axis;
    float phase;
    float speed_rad_per_sec;
};


struct DvzTransformMotionDesc
{
    uint32_t struct_size;
    uint32_t flags;
    const DvzTrack* translation;
    const DvzTrack* rotation;
    const DvzTrack* scale;
    vec3 pivot;
    DvzTransformOrder order;
};


struct DvzCameraMotionDesc
{
    uint32_t struct_size;
    uint32_t flags;
    const DvzTrack* eye;
    const DvzTrack* target;
    DvzCameraUpMode up_mode;
    vec3 up;
    const DvzTrack* up_track;
};



EXTERN_C_ON

/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Set the scene clock mode used by animations.
 *
 * @param scene target scene
 * @param mode realtime or offline clock mode
 */
DVZ_EXPORT void dvz_scene_set_clock_mode(DvzScene* scene, DvzSceneClockMode mode);



/**
 * Set the scene clock frame rate used by offline mode and timer period resolution.
 *
 * @param scene target scene
 * @param fps frames per second, must be positive
 */
DVZ_EXPORT void dvz_scene_set_fps(DvzScene* scene, double fps);



/**
 * Return the current scene clock time in seconds.
 *
 * @param scene target scene
 * @return current scene time
 */
DVZ_EXPORT double dvz_scene_clock_time(const DvzScene* scene);



/**
 * Return the last scene clock delta in seconds.
 *
 * @param scene target scene
 * @return last scene time delta
 */
DVZ_EXPORT double dvz_scene_clock_dt(const DvzScene* scene);


/**
 * Return whether the scene has at least one active animation.
 *
 * @param scene target scene
 * @return true when an animation is active and may need another frame
 */
DVZ_EXPORT bool dvz_scene_has_active_animations(const DvzScene* scene);



/**
 * Create a timer animation driven by the scene clock.
 *
 * @param scene owning scene
 * @param period_s callback period in seconds, or 0 for every scene frame
 * @param callback timer callback
 * @param user_data opaque pointer forwarded to the callback
 * @return the animation handle, or NULL on failure
 */
DVZ_EXPORT DvzAnimation* dvz_anim_timer(
    DvzScene* scene, double period_s, DvzAnimTimerCallback callback, void* user_data);


/**
 * Return a default phase animation descriptor.
 *
 * @return initialized phase animation descriptor
 */
DVZ_EXPORT DvzAnimPhaseDesc dvz_anim_phase_desc(void);


/**
 * Return a default constant track descriptor.
 *
 * @return initialized descriptor
 */
DVZ_EXPORT DvzTrackConstantDesc dvz_track_constant_desc(void);


/**
 * Return a default linear track descriptor.
 *
 * @return initialized descriptor
 */
DVZ_EXPORT DvzTrackLinearDesc dvz_track_linear_desc(void);


/**
 * Return a default keyframe track descriptor.
 *
 * @return initialized descriptor
 */
DVZ_EXPORT DvzTrackKeyframesDesc dvz_track_keyframes_desc(void);


/**
 * Return a default 2D circle track descriptor.
 *
 * @return initialized descriptor
 */
DVZ_EXPORT DvzTrackCircle2Desc dvz_track_circle2_desc(void);


/**
 * Return a default 3D circle track descriptor.
 *
 * @return initialized descriptor
 */
DVZ_EXPORT DvzTrackCircle3Desc dvz_track_circle3_desc(void);


/**
 * Return a default rotation track descriptor.
 *
 * @return initialized descriptor
 */
DVZ_EXPORT DvzTrackRotationDesc dvz_track_rotation_desc(void);


/**
 * Return a default transform motion descriptor.
 *
 * @return initialized descriptor
 */
DVZ_EXPORT DvzTransformMotionDesc dvz_transform_motion_desc(void);


/**
 * Return a default camera motion descriptor.
 *
 * @return initialized descriptor
 */
DVZ_EXPORT DvzCameraMotionDesc dvz_camera_motion_desc(void);


/**
 * Create a constant typed track.
 *
 * @param desc descriptor
 * @return track, or NULL on validation/allocation failure
 */
DVZ_EXPORT DvzTrack* dvz_track_constant(const DvzTrackConstantDesc* desc);


/**
 * Create a linear typed track.
 *
 * @param desc descriptor
 * @return track, or NULL on validation/allocation failure
 */
DVZ_EXPORT DvzTrack* dvz_track_linear(const DvzTrackLinearDesc* desc);


/**
 * Create a keyframed typed track.
 *
 * @param desc descriptor
 * @return track, or NULL on validation/allocation failure
 */
DVZ_EXPORT DvzTrack* dvz_track_keyframes(const DvzTrackKeyframesDesc* desc);


/**
 * Create a 2D circle track.
 *
 * @param desc descriptor
 * @return track, or NULL on validation/allocation failure
 */
DVZ_EXPORT DvzTrack* dvz_track_circle2(const DvzTrackCircle2Desc* desc);


/**
 * Create a 3D circle track.
 *
 * @param desc descriptor
 * @return track, or NULL on validation/allocation failure
 */
DVZ_EXPORT DvzTrack* dvz_track_circle3(const DvzTrackCircle3Desc* desc);


/**
 * Create a quaternion rotation track.
 *
 * @param desc descriptor
 * @return track, or NULL on validation/allocation failure
 */
DVZ_EXPORT DvzTrack* dvz_track_rotation(const DvzTrackRotationDesc* desc);


/**
 * Evaluate a track at local time.
 *
 * @param track track
 * @param t local time in seconds
 * @param out output value with storage matching the track type
 * @return whether evaluation succeeded
 */
DVZ_EXPORT bool dvz_track_eval(const DvzTrack* track, double t, void* out);


/**
 * Destroy a track.
 *
 * @param track track
 */
DVZ_EXPORT void dvz_track_destroy(DvzTrack* track);


/**
 * Create a wrapped linear phase animation driven by the scene clock.
 *
 * @param scene owning scene
 * @param desc phase animation descriptor
 * @return the animation handle, or NULL on failure
 */
DVZ_EXPORT DvzAnimation* dvz_anim_phase(DvzScene* scene, const DvzAnimPhaseDesc* desc);


/**
 * Create a generic track animation driven by the scene clock.
 *
 * @param scene owning scene
 * @param track borrowed track evaluated every frame while active
 * @param callback callback receiving the evaluated value
 * @param user_data opaque pointer forwarded to the callback
 * @return the animation handle, or NULL on failure
 */
DVZ_EXPORT DvzAnimation* dvz_anim_track(
    DvzScene* scene,
    const DvzTrack* track,
    DvzTrackApplyCallback callback,
    void* user_data);


/**
 * Create a visual-local transform animation.
 *
 * @param scene owning scene
 * @param visual visual whose retained local transform is updated
 * @param desc transform motion descriptor
 * @return the animation handle, or NULL on failure
 */
DVZ_EXPORT DvzAnimation* dvz_anim_visual_transform(
    DvzScene* scene, DvzVisual* visual, const DvzTransformMotionDesc* desc);


/**
 * Create a camera motion animation.
 *
 * @param scene owning scene
 * @param camera camera whose view is updated
 * @param desc camera motion descriptor
 * @return the animation handle, or NULL on failure
 */
DVZ_EXPORT DvzAnimation*
dvz_anim_camera_motion(DvzScene* scene, DvzCamera* camera, const DvzCameraMotionDesc* desc);


/**
 * Set how an animation responds to an interactive controller.
 *
 * @param animation animation handle
 * @param controller controller to observe, or NULL to clear policy
 * @param policy interaction policy
 * @param idle_s idle duration for resume-after-idle policies
 */
DVZ_EXPORT void dvz_anim_set_interaction_policy(
    DvzAnimation* animation,
    DvzController* controller,
    DvzAnimInteractionPolicy policy,
    double idle_s);


/**
 * Set the scalar speed used by phase animations, or the local-time multiplier used by track-backed
 * animations.
 *
 * @param animation animation handle
 * @param speed scalar speed in units per second
 */
DVZ_EXPORT void dvz_anim_set_speed(DvzAnimation* animation, float speed);


/**
 * Set the current value of a phase animation.
 *
 * @param animation phase animation handle
 * @param value new phase value, wrapped into the configured interval
 */
DVZ_EXPORT void dvz_anim_phase_set_value(DvzAnimation* animation, float value);



/**
 * Start or restart an animation at a scene-clock time.
 *
 * @param animation animation handle
 * @param t_start scene-clock start time, or 0 for immediate start
 */
DVZ_EXPORT void dvz_anim_start(DvzAnimation* animation, double t_start);



/**
 * Stop an animation while keeping the handle valid.
 *
 * @param animation animation handle
 */
DVZ_EXPORT void dvz_anim_stop(DvzAnimation* animation);



/**
 * Destroy an animation handle owned by its scene.
 *
 * @param animation animation handle
 */
DVZ_EXPORT void dvz_anim_destroy(DvzAnimation* animation);



EXTERN_C_OFF
