/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Orbit camera controller                                                                      */
/*************************************************************************************************/
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

DVZ_EXPORT DvzOrbitCameraDesc dvz_orbit_camera_desc(void);

DVZ_EXPORT DvzOrbitCamera* dvz_orbit_camera_create(const DvzOrbitCameraDesc* desc);

DVZ_EXPORT void dvz_orbit_camera_viewport(
    DvzOrbitCamera* orbit, float x, float y, float width, float height);

DVZ_EXPORT void dvz_orbit_camera_resize(DvzOrbitCamera* orbit, float width, float height);

DVZ_EXPORT void dvz_orbit_camera_pivot(DvzOrbitCamera* orbit, vec3 pivot);

DVZ_EXPORT int dvz_orbit_camera_get_pivot(const DvzOrbitCamera* orbit, vec3 out);

DVZ_EXPORT float dvz_orbit_camera_get_distance(const DvzOrbitCamera* orbit);

DVZ_EXPORT int dvz_orbit_camera_get_view(
    const DvzOrbitCamera* orbit, vec3 eye, vec3 target, vec3 up);

DVZ_EXPORT void dvz_orbit_camera_set_camera(DvzOrbitCamera* orbit, DvzCamera* camera);

DVZ_EXPORT void dvz_orbit_camera_apply_camera(DvzOrbitCamera* orbit);

DVZ_EXPORT bool dvz_orbit_camera_pointer(DvzOrbitCamera* orbit, const DvzPointerEvent* ev);

DVZ_EXPORT bool dvz_orbit_camera_is_interacting(const DvzOrbitCamera* orbit);

DVZ_EXPORT void dvz_orbit_camera_destroy(DvzOrbitCamera* orbit);

EXTERN_C_OFF
