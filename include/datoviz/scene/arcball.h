/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/* Arcball                                                                                       */
/*************************************************************************************************/

#ifndef DVZ_HEADER_ARCBALL
#define DVZ_HEADER_ARCBALL



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "_cglm.h"
#include "_log.h"
#include "datoviz_math.h"
#include "datoviz_types.h"



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzArcball DvzArcball;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzArcball
{
    vec2 viewport_size;
    int flags;

    mat4 mat;        // current model
    vec3 init;       // initial Euler angles
    versor rotation; // current rotation (while dragging), to be applied to mat after dragging
    vec3 constrain;  // constrain axis, null if no constraint

    void* user_data;
};



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 *
 */
DvzArcball* dvz_arcball(float width, float height, int flags); // inner viewport size



/**
 *
 */
bool dvz_arcball_mouse(DvzArcball* arcball, DvzMouseEvent* ev);



/**
 * Destroy an arcball.
 *
 * @param arcball the arcball
 */
void dvz_arcball_destroy(DvzArcball* arcball);



#endif
