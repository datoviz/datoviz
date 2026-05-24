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
    DVZ_ARCBALL_SPIN_FLAGS_NONE                 = 0x00,
    DVZ_ARCBALL_SPIN_FLAGS_PAUSE_ON_INTERACTION = 0x01,
} DvzArcballSpinFlags;



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzAnimation DvzAnimation;
typedef struct DvzArcball DvzArcball;

typedef void (*DvzAnimTimerCallback)(
    DvzAnimation* animation, double t, double dt, void* user_data);

typedef void (*DvzAnimPhaseCallback)(
    DvzAnimation* animation, float value, float delta, void* user_data);



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct DvzAnimPhaseDesc DvzAnimPhaseDesc;

struct DvzAnimPhaseDesc
{
    float initial;
    float speed;
    float wrap_min;
    float wrap_max;
    DvzAnimPhaseCallback callback;
    void* user_data;
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
 * Create a wrapped linear phase animation driven by the scene clock.
 *
 * @param scene owning scene
 * @param desc phase animation descriptor
 * @return the animation handle, or NULL on failure
 */
DVZ_EXPORT DvzAnimation* dvz_anim_phase(DvzScene* scene, const DvzAnimPhaseDesc* desc);


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
DVZ_EXPORT DvzAnimation* dvz_anim_arcball_spin(
    DvzScene* scene, DvzArcball* arcball, vec3 axis, float speed_rad_per_sec, uint32_t flags);


/**
 * Set the scalar speed used by phase and arcball spin animations.
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
