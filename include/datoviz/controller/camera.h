/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Camera                                                                                       */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdint.h>

#include "datoviz/common/macros.h"
#include "datoviz/math/types.h"
#include "datoviz/controller/panzoom.h" /* for DvzMVP */


// Windows headers may define legacy near/far memory-model macros.
#ifdef near
#undef near
#endif
#ifdef far
#undef far
#endif



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/

typedef enum
{
    DVZ_CAMERA_PERSPECTIVE = 0,
    DVZ_CAMERA_ORTHOGRAPHIC = 1,
} DvzCameraType;



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzCamera DvzCamera;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzCameraDesc
{
    uint32_t struct_size;
    uint32_t flags;
    DvzCameraType type;

    vec3 eye;
    vec3 target;
    vec3 up;

    float fov_y;
    float near_clip;
    float far_clip;

    float ortho_height;
};
typedef struct DvzCameraDesc DvzCameraDesc;



EXTERN_C_ON

/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Return a default perspective camera descriptor.
 *
 * @return the camera descriptor
 */
DVZ_EXPORT DvzCameraDesc dvz_camera_desc(void);



/**
 * Create a standalone camera.
 *
 * @param desc the camera descriptor, or NULL for defaults
 * @return the camera, or NULL on allocation failure
 */
DVZ_EXPORT DvzCamera* dvz_camera_create(const DvzCameraDesc* desc);



/**
 * Set a camera view transform.
 *
 * @param camera the camera
 * @param eye the eye position
 * @param target the look-at target
 * @param up the up direction
 */
DVZ_EXPORT void dvz_camera_set_view(DvzCamera* camera, vec3 eye, vec3 target, vec3 up);



/**
 * Return a camera view transform.
 *
 * @param camera the camera
 * @param eye output eye position, or NULL
 * @param target output look-at target, or NULL
 * @param up output up direction, or NULL
 */
DVZ_EXPORT void dvz_camera_get_view(
    const DvzCamera* camera, vec3 eye, vec3 target, vec3 up);



/**
 * Set perspective projection parameters.
 *
 * @param camera the camera
 * @param fov_y vertical field-of-view angle, in radians
 * @param near near clipping plane
 * @param far far clipping plane
 */
DVZ_EXPORT void dvz_camera_set_perspective(
    DvzCamera* camera, float fov_y, float near, float far);



/**
 * Set orthographic projection parameters.
 *
 * @param camera the camera
 * @param height vertical world-space extent
 * @param near near clipping plane
 * @param far far clipping plane
 */
DVZ_EXPORT void dvz_camera_set_orthographic(
    DvzCamera* camera, float height, float near, float far);



/**
 * Update the camera viewport size.
 *
 * @param camera the camera
 * @param width viewport width in pixels
 * @param height viewport height in pixels
 */
DVZ_EXPORT void dvz_camera_resize(DvzCamera* camera, float width, float height);



/**
 * Fill the view and projection matrices of an MVP struct from the camera state.
 *
 * @param camera the camera
 * @param mvp the MVP to update
 */
DVZ_EXPORT void dvz_camera_mvp(DvzCamera* camera, DvzMVP* mvp);



/**
 * Destroy a camera.
 *
 * Use only with standalone cameras returned by `dvz_camera_create()`. Panel-owned cameras returned
 * by `dvz_panel_set_camera()` or `dvz_panel_camera()` are destroyed with their owning panel/scene.
 *
 * @param camera the camera
 */
DVZ_EXPORT void dvz_camera_destroy(DvzCamera* camera);



EXTERN_C_OFF
