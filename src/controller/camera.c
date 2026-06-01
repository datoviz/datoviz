/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Camera                                                                                       */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <math.h>
#include <stdbool.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_controller.h"
#include "_log.h"
#include "datoviz/math/_cglm.h"
#include "datoviz/controller/camera.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define DVZ_CAMERA_DEFAULT_WIDTH 800.0f
#define DVZ_CAMERA_DEFAULT_HEIGHT 600.0f
#define DVZ_CAMERA_DEFAULT_FOV_Y (GLM_PI_4f)
#define DVZ_CAMERA_DEFAULT_NEAR 0.1f
#define DVZ_CAMERA_DEFAULT_FAR 100.0f
#define DVZ_CAMERA_DEFAULT_ORTHO_HEIGHT 2.0f
#define DVZ_CAMERA_DESC_KNOWN_FLAGS 0u



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzCamera
{
    DvzCameraType type;

    vec3 eye;
    vec3 target;
    vec3 up;

    float fov_y;
    float near;
    float far;
    float ortho_height;

    vec2 viewport_size;
    float aspect;
};



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Return whether perspective projection parameters are valid.
 *
 * @param fov_y vertical field-of-view angle
 * @param near near clipping plane
 * @param far far clipping plane
 * @return whether the parameters are valid
 */
static bool _camera_valid_perspective(float fov_y, float near, float far)
{
    return isfinite(fov_y) && isfinite(near) && isfinite(far) && fov_y > 0.0f &&
           fov_y < GLM_PI && near > 0.0f && far > near;
}



/**
 * Return whether orthographic projection parameters are valid.
 *
 * @param height vertical world-space extent
 * @param near near clipping plane
 * @param far far clipping plane
 * @return whether the parameters are valid
 */
static bool _camera_valid_orthographic(float height, float near, float far)
{
    return isfinite(height) && isfinite(near) && isfinite(far) && height > 0.0f && far > near;
}



static bool _camera_desc_validate(const DvzCameraDesc* desc)
{
    if (desc == NULL)
        return true;
    if (!DVZ_STRUCT_VALID(desc, DvzCameraDesc, DVZ_CAMERA_DESC_KNOWN_FLAGS))
    {
        log_error("invalid DvzCameraDesc ABI prologue");
        return false;
    }
    return true;
}



/**
 * Apply a camera descriptor to an initialized camera.
 *
 * @param camera the camera
 * @param desc the camera descriptor
 */
static void _camera_apply_desc(DvzCamera* camera, const DvzCameraDesc* desc)
{
    ANN(camera);
    ANN(desc);
    dvz_camera_set_view(camera, (vec3){desc->eye[0], desc->eye[1], desc->eye[2]},
                        (vec3){desc->target[0], desc->target[1], desc->target[2]},
                        (vec3){desc->up[0], desc->up[1], desc->up[2]});
    if (desc->type == DVZ_CAMERA_ORTHOGRAPHIC)
        dvz_camera_set_orthographic(camera, desc->ortho_height, desc->near, desc->far);
    else
        dvz_camera_set_perspective(camera, desc->fov_y, desc->near, desc->far);
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Return a default perspective camera descriptor.
 *
 * @return the camera descriptor
 */
DvzCameraDesc dvz_camera_desc(void)
{
    return (DvzCameraDesc){
        DVZ_STRUCT_INIT_FIELDS(DvzCameraDesc),
        .type = DVZ_CAMERA_PERSPECTIVE,
        .eye = {0.0f, 0.0f, 3.0f},
        .target = {0.0f, 0.0f, 0.0f},
        .up = {0.0f, 1.0f, 0.0f},
        .fov_y = DVZ_CAMERA_DEFAULT_FOV_Y,
        .near = DVZ_CAMERA_DEFAULT_NEAR,
        .far = DVZ_CAMERA_DEFAULT_FAR,
        .ortho_height = DVZ_CAMERA_DEFAULT_ORTHO_HEIGHT,
    };
}



/**
 * Create a camera from a descriptor.
 *
 * @param desc the camera descriptor
 * @return the camera, or NULL on allocation failure
 */
DvzCamera* dvz_camera_create(const DvzCameraDesc* desc)
{
    DvzCameraDesc default_desc = dvz_camera_desc();
    if (desc == NULL)
        desc = &default_desc;
    if (!_camera_desc_validate(desc))
        return NULL;

    DvzCamera* camera = (DvzCamera*)dvz_calloc(1, sizeof(DvzCamera));
    if (camera == NULL)
        return NULL;
    dvz_camera_resize(camera, DVZ_CAMERA_DEFAULT_WIDTH, DVZ_CAMERA_DEFAULT_HEIGHT);
    _camera_apply_desc(camera, desc);
    return camera;
}


/**
 * Create a camera from a descriptor.
 *
 * @param desc the camera descriptor
 * @return the camera, or NULL on allocation failure
 */
DvzCamera* _dvz_camera(const DvzCameraDesc* desc)
{
    return dvz_camera_create(desc);
}



/**
 * Set a camera view transform.
 *
 * @param camera the camera
 * @param eye the eye position
 * @param target the look-at target
 * @param up the up direction
 */
void dvz_camera_set_view(DvzCamera* camera, vec3 eye, vec3 target, vec3 up)
{
    ANN(camera);
    glm_vec3_copy(eye, camera->eye);
    glm_vec3_copy(target, camera->target);
    glm_vec3_copy(up, camera->up);
}



/**
 * Return a camera view transform.
 *
 * @param camera the camera
 * @param eye output eye position, or NULL
 * @param target output look-at target, or NULL
 * @param up output up direction, or NULL
 */
void dvz_camera_get_view(const DvzCamera* camera, vec3 eye, vec3 target, vec3 up)
{
    ANN(camera);
    for (uint32_t i = 0; i < 3; i++)
    {
        if (eye != NULL)
            eye[i] = camera->eye[i];
        if (target != NULL)
            target[i] = camera->target[i];
        if (up != NULL)
            up[i] = camera->up[i];
    }
}



/**
 * Set perspective projection parameters.
 *
 * @param camera the camera
 * @param fov_y vertical field-of-view angle, in radians
 * @param near near clipping plane
 * @param far far clipping plane
 */
void dvz_camera_set_perspective(DvzCamera* camera, float fov_y, float near, float far)
{
    ANN(camera);
    if (!_camera_valid_perspective(fov_y, near, far))
        return;
    camera->type = DVZ_CAMERA_PERSPECTIVE;
    camera->fov_y = fov_y;
    camera->near = near;
    camera->far = far;
}



/**
 * Set orthographic projection parameters.
 *
 * @param camera the camera
 * @param height vertical world-space extent
 * @param near near clipping plane
 * @param far far clipping plane
 */
void dvz_camera_set_orthographic(DvzCamera* camera, float height, float near, float far)
{
    ANN(camera);
    if (!_camera_valid_orthographic(height, near, far))
        return;
    camera->type = DVZ_CAMERA_ORTHOGRAPHIC;
    camera->ortho_height = height;
    camera->near = near;
    camera->far = far;
}



/**
 * Update the camera viewport size.
 *
 * @param camera the camera
 * @param width viewport width in pixels
 * @param height viewport height in pixels
 */
void dvz_camera_resize(DvzCamera* camera, float width, float height)
{
    ANN(camera);
    if (width <= 0.0f)
        width = DVZ_CAMERA_DEFAULT_WIDTH;
    if (height <= 0.0f)
        height = DVZ_CAMERA_DEFAULT_HEIGHT;
    camera->viewport_size[0] = width;
    camera->viewport_size[1] = height;
    camera->aspect = width / height;
}



/**
 * Fill the view and projection matrices of an MVP struct from the camera state.
 *
 * @param camera the camera
 * @param mvp the MVP to update
 */
void dvz_camera_mvp(DvzCamera* camera, DvzMVP* mvp)
{
    ANN(camera);
    ANN(mvp);
    glm_lookat(camera->eye, camera->target, camera->up, mvp->view);

    if (camera->type == DVZ_CAMERA_ORTHOGRAPHIC)
    {
        float height = camera->ortho_height > 0.0f ? camera->ortho_height :
                                                   DVZ_CAMERA_DEFAULT_ORTHO_HEIGHT;
        float width = height * camera->aspect;
        glm_ortho(
            -0.5f * width, +0.5f * width, -0.5f * height, +0.5f * height, camera->near,
            camera->far, mvp->proj);
    }
    else
    {
        float fov_y = camera->fov_y > 0.0f ? camera->fov_y : DVZ_CAMERA_DEFAULT_FOV_Y;
        glm_perspective(fov_y, camera->aspect, camera->near, camera->far, mvp->proj);
    }
}



/**
 * Destroy a camera.
 *
 * @param camera the camera
 */
void dvz_camera_destroy(DvzCamera* camera)
{
    if (camera == NULL)
        return;
    dvz_free(camera);
}
