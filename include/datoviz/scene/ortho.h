/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/* Ortho                                                                                         */
/*************************************************************************************************/

#ifndef DVZ_HEADER_ORTHO
#define DVZ_HEADER_ORTHO



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "_cglm.h"
#include "_log.h"
#include "datoviz_math.h"
#include "mouse.h"
#include "scene/mvp.h"



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzOrtho DvzOrtho;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzOrtho
{
    vec2 viewport_size;
    int flags;

    vec2 pan;
    vec2 pan_center;
    float zoom;
    vec2 zoom_center;
};



EXTERN_C_ON



/*************************************************************************************************/
/*  Ortho event functions                                                                      */
/*************************************************************************************************/

/**
 *
 */
DvzOrtho* dvz_ortho(float width, float height, int flags); // inner viewport size



/**
 *
 */
bool dvz_ortho_mouse(DvzOrtho* ortho, DvzMouseEvent ev);



/**
 * Function.
 *
 * @param ortho the ortho
 */
void dvz_ortho_destroy(DvzOrtho* ortho);



EXTERN_C_OFF

#endif
