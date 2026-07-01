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
    float near_clip;
    float far_clip;
    float ortho_height;
    bool ortho_has_bounds;
    float ortho_left;
    float ortho_right;
    float ortho_bottom;
    float ortho_top;

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



/**
 * Return whether explicit orthographic bounds are valid.
 *
 * @param left left orthographic bound
 * @param right right orthographic bound
 * @param bottom bottom orthographic bound
 * @param top top orthographic bound
 * @param near near clipping plane
 * @param far far clipping plane
 * @return whether the parameters are valid
 */
static bool _camera_valid_orthographic_bounds(
    float left, float right, float bottom, float top, float near, float far)
{
    return isfinite(left) && isfinite(right) && isfinite(bottom) && isfinite(top) &&
           isfinite(near) && isfinite(far) && left != right && bottom != top && far > near;
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
    dvz_camera_set_view(camera, &desc->view);
    if (desc->projection.type == DVZ_CAMERA_ORTHOGRAPHIC)
    {
        dvz_camera_set_orthographic(
            camera, desc->projection.ortho_height, desc->projection.near_clip,
            desc->projection.far_clip);
    }
    else
    {
        dvz_camera_set_perspective(
            camera, desc->projection.fov_y, desc->projection.near_clip,
            desc->projection.far_clip);
    }
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Return a default camera view.
 *
 * @return the camera view
 */
DvzCameraView dvz_camera_view(void)
{
    return (DvzCameraView){
        .eye = {0.0f, 0.0f, 3.0f},
        .target = {0.0f, 0.0f, 0.0f},
        .up = {0.0f, 1.0f, 0.0f},
    };
}



/**
 * Return a default perspective camera projection.
 *
 * @return the camera projection
 */
DvzCameraProjection dvz_camera_projection(void)
{
    return (DvzCameraProjection){
        .type = DVZ_CAMERA_PERSPECTIVE,
        .fov_y = DVZ_CAMERA_DEFAULT_FOV_Y,
        .near_clip = DVZ_CAMERA_DEFAULT_NEAR,
        .far_clip = DVZ_CAMERA_DEFAULT_FAR,
        .ortho_height = DVZ_CAMERA_DEFAULT_ORTHO_HEIGHT,
    };
}



/**
 * Return a default perspective camera descriptor.
 *
 * @return the camera descriptor
 */
DvzCameraDesc dvz_camera_desc(void)
{
    return (DvzCameraDesc){
        DVZ_STRUCT_INIT_FIELDS(DvzCameraDesc),
        .view = {
            .eye = {0.0f, 0.0f, 3.0f},
            .target = {0.0f, 0.0f, 0.0f},
            .up = {0.0f, 1.0f, 0.0f},
        },
        .projection = {
            .type = DVZ_CAMERA_PERSPECTIVE,
            .fov_y = DVZ_CAMERA_DEFAULT_FOV_Y,
            .near_clip = DVZ_CAMERA_DEFAULT_NEAR,
            .far_clip = DVZ_CAMERA_DEFAULT_FAR,
            .ortho_height = DVZ_CAMERA_DEFAULT_ORTHO_HEIGHT,
        },
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
 * @param view the camera view
 */
void dvz_camera_set_view(DvzCamera* camera, const DvzCameraView* view)
{
    ANN(camera);
    ANN(view);
    for (uint32_t i = 0; i < 3; i++)
    {
        camera->eye[i] = view->eye[i];
        camera->target[i] = view->target[i];
        camera->up[i] = view->up[i];
    }
}



/**
 * Return a camera view transform.
 *
 * @param camera the camera
 * @param out output camera view
 */
void dvz_camera_get_view(const DvzCamera* camera, DvzCameraView* out)
{
    ANN(camera);
    ANN(out);
    for (uint32_t i = 0; i < 3; i++)
    {
        out->eye[i] = camera->eye[i];
        out->target[i] = camera->target[i];
        out->up[i] = camera->up[i];
    }
}


/**
 * Return camera projection parameters.
 *
 * @param camera the camera
 * @param out output camera projection
 */
void dvz_camera_get_projection(const DvzCamera* camera, DvzCameraProjection* out)
{
    ANN(camera);
    ANN(out);
    out->type = camera->type;
    out->fov_y = camera->fov_y;
    out->near_clip = camera->near_clip;
    out->far_clip = camera->far_clip;
    out->ortho_height = camera->ortho_height;
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
    camera->ortho_has_bounds = false;
    camera->fov_y = fov_y;
    camera->near_clip = near;
    camera->far_clip = far;
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
    camera->ortho_has_bounds = false;
    camera->ortho_height = height;
    camera->near_clip = near;
    camera->far_clip = far;
}



/**
 * Set explicit orthographic projection bounds.
 *
 * @param camera the camera
 * @param left left orthographic bound
 * @param right right orthographic bound
 * @param bottom bottom orthographic bound
 * @param top top orthographic bound
 * @param near near clipping plane
 * @param far far clipping plane
 * @return 0 on success, -1 on invalid bounds
 */
int dvz_camera_set_orthographic_bounds(
    DvzCamera* camera, float left, float right, float bottom, float top, float near, float far)
{
    ANN(camera);
    if (!_camera_valid_orthographic_bounds(left, right, bottom, top, near, far))
        return -1;
    camera->type = DVZ_CAMERA_ORTHOGRAPHIC;
    camera->near_clip = near;
    camera->far_clip = far;
    camera->ortho_has_bounds = true;
    camera->ortho_left = left;
    camera->ortho_right = right;
    camera->ortho_bottom = bottom;
    camera->ortho_top = top;
    return 0;
}



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
 * @return 0 when explicit bounds are active, -1 otherwise
 */
int dvz_camera_get_orthographic_bounds(
    const DvzCamera* camera, float* out_left, float* out_right, float* out_bottom, float* out_top,
    float* out_near, float* out_far)
{
    ANN(camera);
    if (!camera->ortho_has_bounds)
        return -1;
    if (out_left != NULL)
        *out_left = camera->ortho_left;
    if (out_right != NULL)
        *out_right = camera->ortho_right;
    if (out_bottom != NULL)
        *out_bottom = camera->ortho_bottom;
    if (out_top != NULL)
        *out_top = camera->ortho_top;
    if (out_near != NULL)
        *out_near = camera->near_clip;
    if (out_far != NULL)
        *out_far = camera->far_clip;
    return 0;
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
        if (camera->ortho_has_bounds)
        {
            glm_ortho(
                camera->ortho_left, camera->ortho_right, camera->ortho_bottom, camera->ortho_top,
                camera->near_clip, camera->far_clip, mvp->proj);
        }
        else
        {
            float height = camera->ortho_height > 0.0f ? camera->ortho_height :
                                                       DVZ_CAMERA_DEFAULT_ORTHO_HEIGHT;
            float width = height * camera->aspect;
            glm_ortho(
                -0.5f * width, +0.5f * width, -0.5f * height, +0.5f * height, camera->near_clip,
                camera->far_clip, mvp->proj);
        }
    }
    else
    {
        float fov_y = camera->fov_y > 0.0f ? camera->fov_y : DVZ_CAMERA_DEFAULT_FOV_Y;
        glm_perspective(fov_y, camera->aspect, camera->near_clip, camera->far_clip, mvp->proj);
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
