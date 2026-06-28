/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Turntable controller                                                                         */
/*************************************************************************************************/
/* Advanced/unstable standalone controller internals. Prefer scene-owned controllers for ordinary
 * v0.4 scene/app use. */
/* World-up constrained camera orbit around a pivot. Intended for upright object or scene
 * inspection where yaw, pitch/elevation, distance, and optional pivot pan remain predictable. */

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>

#include "datoviz/common/macros.h"
#include "datoviz/input/pointer.h"
#include "datoviz/input/router.h"
#include "datoviz/math/types.h"
#include "datoviz/controller/camera.h"



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/

typedef enum
{
    DVZ_TURNTABLE_FLAGS_NONE           = 0x00,
    DVZ_TURNTABLE_FLAGS_INVERT_Y       = 0x01,
    DVZ_TURNTABLE_FLAGS_ALLOW_PAN      = 0x02,
    DVZ_TURNTABLE_FLAGS_WRAP_YAW       = 0x04,
    DVZ_TURNTABLE_FLAGS_CLAMP_DISTANCE = 0x08,
} DvzTurntableFlags;



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzTurntableDesc DvzTurntableDesc;
typedef struct DvzTurntable DvzTurntable;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzTurntableDesc
{
    uint32_t struct_size;
    uint32_t flags;
    DvzCameraView initial_view;

    float yaw_speed;
    float pitch_speed;
    float zoom_speed;
    float pan_speed;

    float min_pitch;
    float max_pitch;
    float min_distance;
    float max_distance;

    uint32_t controller_flags;
};



struct DvzTurntable
{
    int flags;

    vec2 viewport_origin;
    vec2 viewport_size;
    bool has_viewport;
    bool interacting;

    vec3 pivot;
    vec3 eye;
    vec3 up;

    vec3 pivot_init;
    float distance_init;
    float yaw_init;
    float pitch_init;

    float distance;
    float yaw;
    float pitch;

    float yaw_speed;
    float pitch_speed;
    float zoom_speed;
    float pan_speed;

    float min_pitch;
    float max_pitch;
    float min_distance;
    float max_distance;

    bool pivot_marker_visible;
    double pivot_marker_time_left;

    DvzCamera* camera;
};



EXTERN_C_ON

/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Return a default turntable descriptor.
 *
 * @return the turntable descriptor
 */
DVZ_EXPORT DvzTurntableDesc dvz_turntable_desc(void);



/**
 * Create a standalone turntable controller.
 *
 * @param desc descriptor, or NULL for defaults
 * @return the controller, or NULL on allocation failure
 */
DVZ_EXPORT DvzTurntable* dvz_turntable_create(const DvzTurntableDesc* desc);



/**
 * Reset a turntable to its initial pose.
 *
 * @param turntable the turntable controller
 */
DVZ_EXPORT void dvz_turntable_reset(DvzTurntable* turntable);



/**
 * Update the viewport rectangle in window coordinates.
 *
 * @param turntable the turntable controller
 * @param x viewport x origin in window pixels
 * @param y viewport y origin in window pixels
 * @param width viewport width in window pixels
 * @param height viewport height in window pixels
 */
DVZ_EXPORT void
dvz_turntable_viewport(DvzTurntable* turntable, float x, float y, float width, float height);



/**
 * Update the viewport size.
 *
 * @param turntable the turntable controller
 * @param width viewport width in pixels
 * @param height viewport height in pixels
 */
DVZ_EXPORT void dvz_turntable_resize(DvzTurntable* turntable, float width, float height);



/**
 * Set the pivot while preserving the current camera eye.
 *
 * @param turntable the turntable controller
 * @param pivot new world-space pivot
 */
DVZ_EXPORT void dvz_turntable_pivot(DvzTurntable* turntable, vec3 pivot);



/**
 * Orbit around the pivot.
 *
 * @param turntable the turntable controller
 * @param yaw_delta yaw delta in radians
 * @param pitch_delta pitch delta in radians
 */
DVZ_EXPORT void
dvz_turntable_orbit(DvzTurntable* turntable, float yaw_delta, float pitch_delta);



/**
 * Dolly toward or away from the pivot.
 *
 * @param turntable the turntable controller
 * @param amount distance delta
 */
DVZ_EXPORT void dvz_turntable_dolly(DvzTurntable* turntable, float amount);



/**
 * Pan the pivot in the current view plane.
 *
 * @param turntable the turntable controller
 * @param right_amount right-axis pan amount
 * @param up_amount up-axis pan amount
 */
DVZ_EXPORT void
dvz_turntable_pan(DvzTurntable* turntable, float right_amount, float up_amount);



/**
 * Attach a camera updated by this turntable.
 *
 * @param turntable the turntable controller
 * @param camera the camera to update, or NULL
 */
DVZ_EXPORT void dvz_turntable_set_camera(DvzTurntable* turntable, DvzCamera* camera);



/**
 * Apply the turntable pose to the attached camera.
 *
 * @param turntable the turntable controller
 */
DVZ_EXPORT void dvz_turntable_apply_camera(DvzTurntable* turntable);



/**
 * Process a pointer event.
 *
 * @param turntable the turntable controller
 * @param ev pointer event
 * @return true if the event was consumed
 */
DVZ_EXPORT bool dvz_turntable_pointer(DvzTurntable* turntable, const DvzPointerEvent* ev);



/**
 * Subscribe the turntable to an input router.
 *
 * @param turntable the turntable controller
 * @param router input router
 */
DVZ_EXPORT void dvz_turntable_connect(DvzTurntable* turntable, DvzInputRouter* router);



/**
 * Unsubscribe the turntable from an input router.
 *
 * @param turntable the turntable controller
 * @param router input router
 */
DVZ_EXPORT void dvz_turntable_disconnect(DvzTurntable* turntable, DvzInputRouter* router);



/**
 * Destroy a turntable controller.
 *
 * @param turntable the turntable controller
 */
DVZ_EXPORT void dvz_turntable_destroy(DvzTurntable* turntable);



EXTERN_C_OFF
