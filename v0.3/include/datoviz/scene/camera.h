/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/* Camera                                                                                        */
/*************************************************************************************************/

#ifndef DVZ_HEADER_CAMERA
#define DVZ_HEADER_CAMERA



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "_cglm.h"
#include "_log.h"
#include "datoviz_defaults.h"
#include "datoviz_math.h"
#include "datoviz_types.h"
#include "scene/camera.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

// HACK: work around mingw64 gcc warning on Windows
// see https://stackoverflow.com/a/2754992/1595060
#if OS_WINDOWS
#undef near
#undef far
#endif


/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzCamera DvzCamera;



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzCamera
{
    vec2 viewport_size;
    int flags;

    // zrange
    float near;
    float far;

    float aspect;
    // TODO: use type and union?
    // for orthographic camera
    float left, right, bottom, top;

    // for perspective camera
    vec3 pos, lookat, up;
    vec3 pos_init, lookat_init, up_init; // initial camera parameters
    float fov;
};



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 *
 */
DvzCamera* dvz_camera(float width, float height, int flags);



/**
 * Function.
 *
 * @param camera the camera
 */
void dvz_camera_destroy(DvzCamera* camera);



#endif
