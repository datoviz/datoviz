/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Orbit camera controller                                                                      */
/*************************************************************************************************/
/* Advanced/unstable standalone controller internals. Prefer scene-owned controllers for ordinary
 * v0.4 scene/app use. */
/* Camera-state orbit around a pivot. Intended for camera-centric navigation where the attached
 * camera pose is the primary state and the pivot defines the temporary orbit target. */

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
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzOrbitCameraDesc DvzOrbitCameraDesc;
typedef struct DvzOrbitCamera DvzOrbitCamera;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzOrbitCameraDesc
{
    uint32_t struct_size;
    uint32_t flags;
    float width;
    float height;
    uint32_t controller_flags;
    vec3 pivot;
    float min_distance;
    float max_distance;
    float zoom_speed;
};



EXTERN_C_ON

/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Return the default orbit-camera descriptor.
 *
 * @return default descriptor
 */
DVZ_EXPORT DvzOrbitCameraDesc dvz_orbit_camera_desc(void);

/**
 * Create a standalone orbit-camera controller.
 *
 * @param desc descriptor, or NULL for defaults
 * @return controller, or NULL on allocation failure
 */
DVZ_EXPORT DvzOrbitCamera* dvz_orbit_camera_create(const DvzOrbitCameraDesc* desc);

/**
 * Set the viewport rectangle in window coordinates.
 *
 * @param orbit the orbit-camera controller
 * @param x viewport origin x in pixels
 * @param y viewport origin y in pixels
 * @param width viewport width in pixels
 * @param height viewport height in pixels
 */
DVZ_EXPORT void dvz_orbit_camera_viewport(
    DvzOrbitCamera* orbit, float x, float y, float width, float height);

/**
 * Update the viewport size.
 *
 * @param orbit the orbit-camera controller
 * @param width viewport width in pixels
 * @param height viewport height in pixels
 */
DVZ_EXPORT void dvz_orbit_camera_resize(DvzOrbitCamera* orbit, float width, float height);

/**
 * Set the orbit pivot in world coordinates.
 *
 * @param orbit the orbit-camera controller
 * @param pivot pivot point
 */
DVZ_EXPORT void dvz_orbit_camera_pivot(DvzOrbitCamera* orbit, vec3 pivot);

/**
 * Copy the orbit pivot.
 *
 * @param orbit the orbit-camera controller
 * @param[out] out pivot point
 * @return 0 on success, -1 on validation error
 */
DVZ_EXPORT int dvz_orbit_camera_get_pivot(const DvzOrbitCamera* orbit, vec3 out);

/**
 * Return the current camera-to-pivot distance.
 *
 * @param orbit the orbit-camera controller
 * @return distance, or 0 when orbit is NULL
 */
DVZ_EXPORT float dvz_orbit_camera_get_distance(const DvzOrbitCamera* orbit);

/**
 * Copy the current camera view vectors.
 *
 * @param orbit the orbit-camera controller
 * @param[out] eye camera eye position
 * @param[out] target camera target position
 * @param[out] up camera up vector
 * @return 0 on success, -1 on validation error
 */
DVZ_EXPORT int dvz_orbit_camera_get_view(
    const DvzOrbitCamera* orbit, vec3 eye, vec3 target, vec3 up);

/**
 * Attach a borrowed camera to the controller.
 *
 * The camera remains owned by its creator or panel. The orbit controller updates it when
 * `dvz_orbit_camera_apply_camera()` runs and never destroys it.
 *
 * @param orbit the orbit-camera controller
 * @param camera borrowed camera, or NULL to detach
 */
DVZ_EXPORT void dvz_orbit_camera_set_camera(DvzOrbitCamera* orbit, DvzCamera* camera);

/**
 * Apply the orbit view to the attached borrowed camera.
 *
 * @param orbit the orbit-camera controller
 */
DVZ_EXPORT void dvz_orbit_camera_apply_camera(DvzOrbitCamera* orbit);

/**
 * Process a pointer event and update orbit state.
 *
 * @param orbit the orbit-camera controller
 * @param ev pointer event
 * @return true if the event was consumed
 */
DVZ_EXPORT bool dvz_orbit_camera_pointer(DvzOrbitCamera* orbit, const DvzPointerEvent* ev);

/**
 * Return whether the pointer is currently interacting with the controller.
 *
 * @param orbit the orbit-camera controller
 * @return true while the user is pressing or dragging the orbit controller
 */
DVZ_EXPORT bool dvz_orbit_camera_is_interacting(const DvzOrbitCamera* orbit);

/**
 * Destroy a standalone orbit-camera controller.
 *
 * Attached borrowed cameras are not destroyed.
 *
 * @param orbit the orbit-camera controller
 */
DVZ_EXPORT void dvz_orbit_camera_destroy(DvzOrbitCamera* orbit);

EXTERN_C_OFF
