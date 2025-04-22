/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/* Image                                                                                         */
/*************************************************************************************************/

#ifndef DVZ_HEADER_IMAGE
#define DVZ_HEADER_IMAGE



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "../viewport.h"
#include "../visual.h"



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzImageVertex DvzImageVertex;
typedef struct DvzImageParams DvzImageParams;



/*************************************************************************************************/
/*  Enums                                                                                     */
/*************************************************************************************************/

typedef enum
{
    DVZ_IMAGE_PARAMS_EDGECOLOR,
    DVZ_IMAGE_PARAMS_PERMUTATION,
    DVZ_IMAGE_PARAMS_LINEWIDTH,
    DVZ_IMAGE_PARAMS_RADIUS,
    DVZ_IMAGE_PARAMS_COLORMAP,
} DvzImageParamsEnum;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzImageVertex
{
    vec3 pos;           /* position */
    vec2 size;          /* size */
    vec2 anchor;        /* anchor */
    vec2 uv;            /* texture coordinates */
    DvzColor facecolor; /* face color (in FILL mode) */
};



struct DvzImageParams
{
    vec4 edgecolor;    /* color of the border */
    ivec2 permutation; /* (0,1) by default */
    float linewidth;   /* width of the border, 0 for no border */
    float radius;      /* rounded rectangle radius, 0 for sharp corners */
    int cmap;
};



#endif
