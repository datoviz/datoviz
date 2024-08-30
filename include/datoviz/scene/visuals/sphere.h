/*
* Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
* Licensed under the MIT license. See LICENSE file in the project root for details.
* SPDX-License-Identifier: MIT
*/

/*************************************************************************************************/
/* Sphere                                                                                    */
/*************************************************************************************************/

#ifndef DVZ_HEADER_SPHERE
#define DVZ_HEADER_SPHERE



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "../viewport.h"
#include "../visual.h"



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzSphereVertex DvzSphereVertex;
typedef struct DvzSphereParams DvzSphereParams;

// Forward declarations.
typedef struct DvzBatch DvzBatch;
typedef struct DvzVisual DvzVisual;



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzSphereVertex
{
    vec3 pos;    /* position */
    cvec4 color; /* color */
    float size;  /* size */
};



struct DvzSphereParams
{
    vec4 light_pos;   /* light position */
    vec4 light_param; /* ambient, diffuse, specular coefs */
};



#endif
