/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/* Mesh                                                                                          */
/*************************************************************************************************/

#ifndef DVZ_HEADER_MESH
#define DVZ_HEADER_MESH



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "../viewport.h"
#include "../visual.h"



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzMeshColorVertex DvzMeshColorVertex;
typedef struct DvzMeshTexturedVertex DvzMeshTexturedVertex;
typedef struct DvzMeshParams DvzMeshParams;

// Forward declarations.
typedef struct DvzBatch DvzBatch;
typedef struct DvzVisual DvzVisual;
typedef struct DvzShape DvzShape;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzMeshColorVertex
{
    // HACK: use vec4 for alignment when accessing from compute shader (need std140 on GPU)
    vec3 pos;       /* position */
    vec3 normal;    /* normal vector */
    DvzColor color; /* color rgba */
    float value;    /* scalar value used for isolines */
    vec3 d_left;    /* distance of the current vertex between the left edge and point A, B, C */
    vec3 d_right;   /* distance of the current vertex between the right edge and point A, B, C */
    cvec4 contour;  /* 0bXY where Y=1 if the opposite edge is a contour, X=1 if vertex is corner */
};

struct DvzMeshTexturedVertex
{
    // HACK: use vec4 for alignment when accessing from compute shader (need std140 on GPU)
    vec3 pos;       /* position */
    vec3 normal;    /* normal vector */
    vec4 texcoords; /* u, v, *, a */
    float value;    /* scalar value used for isolines */
    vec3 d_left;    /* distance of the current vertex between the left edge and point A, B, C */
    vec3 d_right;   /* distance of the current vertex between the right edge and point A, B, C */
    cvec4 contour;  /* 0bXY where Y=1 if the opposite edge is a contour, X=1 if vertex is corner */
};



struct DvzMeshParams
{
    mat4 light_dir;         /* x, y, z, *** */
    mat4 light_color;       /* r, g, b, *** */
    mat4 light_params;      /* ambient, diffuse, specular, exponent */
    vec4 stroke;            /* r, g, b, stroke width */
    uint32_t isoline_count; /* number of isolines */
};



typedef enum
{
    DVZ_MESH_PARAMS_LIGHT_DIR,
    DVZ_MESH_PARAMS_LIGHT_COLOR,
    DVZ_MESH_PARAMS_LIGHT_PARAMS,
    DVZ_MESH_PARAMS_STROKE,
    DVZ_MESH_PARAMS_ISOLINE_COUNT,
} DvzMeshParamsEnum;



#endif
