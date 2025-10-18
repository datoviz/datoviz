/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/* Wiggle                                                                                        */
/*************************************************************************************************/

#ifndef DVZ_HEADER_WIGGLE
#define DVZ_HEADER_WIGGLE



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "../viewport.h"
#include "../visual.h"



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzWiggleVertex DvzWiggleVertex;
typedef struct DvzWiggleParams DvzWiggleParams;



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/

typedef enum
{
    DVZ_WIGGLE_PARAMS_NEGATIVE_COLOR,
    DVZ_WIGGLE_PARAMS_POSITIVE_COLOR,
    DVZ_WIGGLE_PARAMS_EDGECOLOR,
    DVZ_WIGGLE_PARAMS_XRANGE,
    DVZ_WIGGLE_PARAMS_CHANNELS,
    DVZ_WIGGLE_PARAMS_SCALE,
} DvzWiggleParamsEnum;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzWiggleVertex
{
    vec3 pos; /* position */
    vec2 uv;  /* texture coordinates */
};



struct DvzWiggleParams
{
    vec4 negative_color;
    vec4 positive_color;
    vec4 edgecolor;
    vec2 xrange;
    int channels;
    float scale;
    // int swizzle; // TODO: generic swizzle system with spec constants (planned for v0.4)
};



#endif
