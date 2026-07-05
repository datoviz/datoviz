/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Fly camera controller                                                                        */
/*************************************************************************************************/
/* Advanced/unstable standalone controller internals. Prefer scene-owned controllers for ordinary
 * v0.4 scene/app use. */

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>

#include "datoviz/common/macros.h"
#include "datoviz/common/types.h"
#include "datoviz/input/keyboard.h"
#include "datoviz/input/pointer.h"
#include "datoviz/input/router.h"
#include "datoviz/math/types.h"
#include "datoviz/controller/camera.h"



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/

typedef enum
{
    DVZ_FLY_MODE_FREE = 0,
    DVZ_FLY_MODE_PLANE = 1,
} DvzFlyMode;



typedef enum
{
    DVZ_FLY_FLAGS_NONE         = 0x00,
    DVZ_FLY_FLAGS_INVERT_Y     = 0x01,
    DVZ_FLY_FLAGS_FIXED_UP     = 0x02,
    DVZ_FLY_FLAGS_DISABLE_ROLL = 0x04,
} DvzFlyFlags;



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzFlyDesc DvzFlyDesc;
typedef struct DvzFly DvzFly;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzFlyDesc
{
    uint32_t struct_size;
    uint32_t flags;
    DvzFlyMode mode;
    uint32_t controller_flags;

    DvzCameraView initial_view;

    float yaw;
    float pitch;
    float roll;
    bool use_angles;

    float speed;
    float fast_multiplier;
    float slow_multiplier;
    float look_speed;
    float wheel_speed;
};



EXTERN_C_ON

/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Return a default fly-controller descriptor.
 *
 * @return the fly descriptor
 */
DVZ_EXPORT DvzFlyDesc dvz_fly_desc(void);



/**
 * Create a standalone fly camera controller.
 *
 * @param desc fly descriptor, or NULL for defaults
 * @return the controller, or NULL on allocation failure
 */
DVZ_EXPORT DvzFly* dvz_fly_create(const DvzFlyDesc* desc);



/**
 * Reset a fly controller to its initial pose.
 *
 * @param fly the fly controller
 */
DVZ_EXPORT DvzResult dvz_fly_reset(DvzFly* fly);



/**
 * Update the viewport rectangle in window coordinates.
 *
 * @param fly the fly controller
 * @param x viewport x origin in window pixels
 * @param y viewport y origin in window pixels
 * @param width viewport width in window pixels
 * @param height viewport height in window pixels
 */
DVZ_EXPORT DvzResult dvz_fly_viewport(DvzFly* fly, float x, float y, float width, float height);



/**
 * Update the viewport size.
 *
 * @param fly the fly controller
 * @param width viewport width in pixels
 * @param height viewport height in pixels
 */
DVZ_EXPORT DvzResult dvz_fly_resize(DvzFly* fly, float width, float height);



/**
 * Set the initial pose from angles and reset.
 *
 * @param fly the fly controller
 * @param position initial camera position
 * @param yaw initial yaw angle in radians
 * @param pitch initial pitch angle in radians
 * @param roll initial roll angle in radians
 */
DVZ_EXPORT DvzResult dvz_fly_initial(DvzFly* fly, vec3 position, float yaw, float pitch, float roll);



/**
 * Set the initial pose from a look-at point and reset.
 *
 * @param fly the fly controller
 * @param position initial camera position
 * @param target initial look-at target
 */
DVZ_EXPORT DvzResult dvz_fly_initial_lookat(DvzFly* fly, vec3 position, vec3 target);



/**
 * Set the movement mode.
 *
 * @param fly the fly controller
 * @param mode movement mode
 */
DVZ_EXPORT DvzResult dvz_fly_set_mode(DvzFly* fly, DvzFlyMode mode);



/**
 * Move forward along the active movement direction.
 *
 * @param fly the fly controller
 * @param amount movement amount in world units
 */
DVZ_EXPORT DvzResult dvz_fly_move_forward(DvzFly* fly, float amount);



/**
 * Move right relative to the active movement direction.
 *
 * @param fly the fly controller
 * @param amount movement amount in world units
 */
DVZ_EXPORT DvzResult dvz_fly_move_right(DvzFly* fly, float amount);



/**
 * Move up along the fly controller's world-up direction.
 *
 * @param fly the fly controller
 * @param amount movement amount in world units
 */
DVZ_EXPORT DvzResult dvz_fly_move_up(DvzFly* fly, float amount);



/**
 * Rotate the fly view direction.
 *
 * @param fly the fly controller
 * @param dx yaw delta in radians
 * @param dy pitch delta in radians
 */
DVZ_EXPORT DvzResult dvz_fly_rotate(DvzFly* fly, float dx, float dy);



/**
 * Roll the fly camera around its view direction.
 *
 * @param fly the fly controller
 * @param dx roll delta in radians
 */
DVZ_EXPORT DvzResult dvz_fly_roll(DvzFly* fly, float dx);



/**
 * Return the current position.
 *
 * @param fly the fly controller
 * @param out_pos output position
 */
DVZ_EXPORT void dvz_fly_get_position(const DvzFly* fly, vec3 out_pos);



/**
 * Return the current look-at target.
 *
 * @param fly the fly controller
 * @param out_target output target
 */
DVZ_EXPORT void dvz_fly_get_target(const DvzFly* fly, vec3 out_target);



/**
 * Return the current up vector.
 *
 * @param fly the fly controller
 * @param out_up output up vector
 */
DVZ_EXPORT void dvz_fly_get_up(const DvzFly* fly, vec3 out_up);



/**
 * Set or move the optional orbit pivot while preserving the camera eye.
 *
 * @param fly the fly controller
 * @param pivot new world-space pivot point
 */
DVZ_EXPORT DvzResult dvz_fly_pivot(DvzFly* fly, vec3 pivot);



/**
 * Clear the optional orbit pivot.
 *
 * @param fly the fly controller
 */
DVZ_EXPORT DvzResult dvz_fly_clear_pivot(DvzFly* fly);



/**
 * Return whether an orbit pivot is set.
 *
 * @param fly the fly controller
 * @return whether the fly controller has a pivot
 */
DVZ_EXPORT bool dvz_fly_has_pivot(const DvzFly* fly);



/**
 * Reorient the camera toward the active pivot without moving the eye.
 *
 * @param fly the fly controller
 */
DVZ_EXPORT DvzResult dvz_fly_look_at_pivot(DvzFly* fly);



/**
 * Orbit the camera around the active pivot.
 *
 * @param fly the fly controller
 * @param yaw_delta yaw delta in radians
 * @param pitch_delta pitch delta in radians
 * @return whether the orbit was applied
 */
DVZ_EXPORT DvzResult dvz_fly_orbit(DvzFly* fly, float yaw_delta, float pitch_delta);



/**
 * Attach a camera updated by this fly controller.
 *
 * @param fly the fly controller
 * @param camera the camera to update, or NULL
 */
DVZ_EXPORT DvzResult dvz_fly_set_camera(DvzFly* fly, DvzCamera* camera);



/**
 * Apply the current fly pose to the attached camera.
 *
 * @param fly the fly controller
 */
DVZ_EXPORT DvzResult dvz_fly_apply_camera(DvzFly* fly);



/**
 * Advance held-key movement.
 *
 * @param fly the fly controller
 * @param dt elapsed time in seconds
 */
DVZ_EXPORT DvzResult dvz_fly_update(DvzFly* fly, double dt);



/**
 * Process a pointer event.
 *
 * @param fly the fly controller
 * @param ev pointer event
 * @return true if the event was consumed
 */
DVZ_EXPORT bool dvz_fly_pointer(DvzFly* fly, const DvzPointerEvent* ev);



/**
 * Process a keyboard event.
 *
 * @param fly the fly controller
 * @param ev keyboard event
 * @return true if the event was consumed
 */
DVZ_EXPORT bool dvz_fly_keyboard(DvzFly* fly, const DvzKeyboardEvent* ev);



/**
 * Subscribe the fly controller to an input router.
 *
 * @param fly the fly controller
 * @param router input router
 */
DVZ_EXPORT DvzResult dvz_fly_connect(DvzFly* fly, DvzInputRouter* router);



/**
 * Unsubscribe the fly controller from an input router.
 *
 * @param fly the fly controller
 * @param router input router
 */
DVZ_EXPORT DvzResult dvz_fly_disconnect(DvzFly* fly, DvzInputRouter* router);



/**
 * Destroy a fly controller.
 *
 * @param fly the fly controller
 */
DVZ_EXPORT void dvz_fly_destroy(DvzFly* fly);



EXTERN_C_OFF
