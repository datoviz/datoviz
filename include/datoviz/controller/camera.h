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
#include "datoviz/common/types.h"
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

struct DvzCameraView
{
    vec3 eye;
    vec3 target;
    vec3 up;
};
typedef struct DvzCameraView DvzCameraView;



struct DvzCameraProjection
{
    DvzCameraType type;
    float fov_y;
    float near_clip;
    float far_clip;
    float ortho_height;
};
typedef struct DvzCameraProjection DvzCameraProjection;



struct DvzCameraDesc
{
    uint32_t struct_size;
    uint32_t flags;
    DvzCameraView view;
    DvzCameraProjection projection;
};
typedef struct DvzCameraDesc DvzCameraDesc;



EXTERN_C_ON

/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Return a default camera view.
 *
 * @return the camera view
 */
DVZ_EXPORT DvzCameraView dvz_camera_view(void);



/**
 * Return a default perspective camera projection.
 *
 * @return the camera projection
 */
DVZ_EXPORT DvzCameraProjection dvz_camera_projection(void);



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
 * @param view the camera view
 * @return DVZ_OK on success, DVZ_ERROR on validation error
 */
DVZ_EXPORT DvzResult dvz_camera_set_view(DvzCamera* camera, const DvzCameraView* view);



/**
 * Return a camera view transform.
 *
 * @param camera the camera
 * @param out output camera view
 */
DVZ_EXPORT void dvz_camera_get_view(const DvzCamera* camera, DvzCameraView* out);


/**
 * Return camera projection parameters.
 *
 * @param camera the camera
 * @param out output camera projection
 */
DVZ_EXPORT void dvz_camera_get_projection(const DvzCamera* camera, DvzCameraProjection* out);



/**
 * Set perspective projection parameters.
 *
 * @param camera the camera
 * @param fov_y vertical field-of-view angle, in radians
 * @param near near clipping plane
 * @param far far clipping plane
 * @return DVZ_OK on success, DVZ_ERROR on validation error
 */
DVZ_EXPORT DvzResult dvz_camera_set_perspective(
    DvzCamera* camera, float fov_y, float near, float far);



/**
 * Set orthographic projection parameters.
 *
 * @param camera the camera
 * @param height vertical world-space extent
 * @param near near clipping plane
 * @param far far clipping plane
 * @return DVZ_OK on success, DVZ_ERROR on validation error
 */
DVZ_EXPORT DvzResult dvz_camera_set_orthographic(
    DvzCamera* camera, float height, float near, float far);



/**
 * Set explicit orthographic projection bounds.
 *
 * This preserves the bounds exactly as supplied, including reversed left/right or bottom/top
 * pairs. Unlike `dvz_camera_set_orthographic()`, these bounds are not rewritten on resize.
 *
 * @param camera the camera
 * @param left left orthographic bound
 * @param right right orthographic bound
 * @param bottom bottom orthographic bound
 * @param top top orthographic bound
 * @param near near clipping plane
 * @param far far clipping plane
 * @return DVZ_OK on success, DVZ_ERROR on validation error
 */
DVZ_EXPORT DvzResult dvz_camera_set_orthographic_bounds(
    DvzCamera* camera, float left, float right, float bottom, float top, float near, float far);



/**
 * Return explicit orthographic projection bounds.
 *
 * @param camera the camera
 * @param out_left output left orthographic bound
 * @param out_right output right orthographic bound
 * @param out_bottom output bottom orthographic bound
 * @param out_top output top orthographic bound
 * @param out_near output near clipping plane
 * @param out_far output far clipping plane
 * @return DVZ_OK when explicit bounds are active, DVZ_ERROR otherwise
 */
DVZ_EXPORT DvzResult dvz_camera_get_orthographic_bounds(
    const DvzCamera* camera, float* out_left, float* out_right, float* out_bottom, float* out_top,
    float* out_near, float* out_far);



/**
 * Update the camera viewport size.
 *
 * @param camera the camera
 * @param width viewport width in pixels
 * @param height viewport height in pixels
 * @return DVZ_OK on success, DVZ_ERROR on validation error
 */
DVZ_EXPORT DvzResult dvz_camera_resize(DvzCamera* camera, float width, float height);



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
 * by `dvz_panel_set_camera_desc()` or `dvz_panel_camera()` are destroyed with their owning panel/scene.
 *
 * @param camera the camera
 */
DVZ_EXPORT void dvz_camera_destroy(DvzCamera* camera);



EXTERN_C_OFF
