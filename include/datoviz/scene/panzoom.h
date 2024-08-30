/*
* Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
* Licensed under the MIT license. See LICENSE file in the project root for details.
* SPDX-License-Identifier: MIT
*/

/*************************************************************************************************/
/* Panzoom                                                                                       */
/*************************************************************************************************/

#ifndef DVZ_HEADER_PANZOOM
#define DVZ_HEADER_PANZOOM



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

typedef struct DvzPanzoom DvzPanzoom;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzPanzoom
{
    vec2 viewport_size;
    vec2 xlim;
    vec2 ylim;
    vec2 zlim;
    int flags;

    vec2 pan;
    vec2 pan_center;
    vec2 zoom;
    vec2 zoom_center;
    // DvzMVP mvp;
};



EXTERN_C_ON



/*************************************************************************************************/
/*  Panzoom event functions                                                                      */
/*************************************************************************************************/

/**
 *
 */
DvzPanzoom* dvz_panzoom(float width, float height, int flags); // inner viewport size



/**
 *
 */
bool dvz_panzoom_mouse(DvzPanzoom* pz, DvzMouseEvent ev);



/**
 * Function.
 *
 * @param pz the pz
 */
void dvz_panzoom_destroy(DvzPanzoom* pz);



EXTERN_C_OFF

#endif
