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
    int flags;

    vec2 pan;
    vec2 pan_center;
    vec2 zoom;
    vec2 zoom_center;

    // Lock.
    vec2 pan_lock;
    vec2 zoom_lock;
    bool pan_locked[2];
    bool zoom_locked[2];
};



EXTERN_C_ON



EXTERN_C_OFF

#endif
