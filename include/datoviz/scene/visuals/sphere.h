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
typedef struct DvzSphereLight DvzSphereLight;
typedef struct DvzSphereMaterial DvzSphereMaterial;

// Forward declarations.
typedef struct DvzBatch DvzBatch;
typedef struct DvzVisual DvzVisual;



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/

typedef enum
{
    DVZ_LIGHT_PARAMS_POS,
    DVZ_LIGHT_PARAMS_COLOR,
} DvzLightParamsEnum;

typedef enum
{
    DVZ_SPHERE_PARAMS_PARAMS,
    DVZ_SPHERE_PARAMS_SHINE,
    DVZ_SPHERE_PARAMS_EMIT,
} DvzSphereParamsEnum;


/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzSphereVertex
{
    vec3 pos;       /* position */
    DvzColor color; /* color */
    float size;     /* size */
};

struct DvzSphereLight
{
    mat4 pos;   // w=0 indicates it's a direction with no position.
    mat4 color; // alpha value indicates it's on.
};

struct DvzSphereMaterial
{
    mat4 params; /* (r, g, b, -) X (ambient, specular, diffuse, emission) */
    float shine; /* specular amount */
    float emit;  /* emission level */
};



#endif
