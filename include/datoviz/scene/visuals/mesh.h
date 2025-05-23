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
typedef struct DvzMeshLight DvzMeshLight;
typedef struct DvzMeshMaterial DvzMeshMaterial;
typedef struct DvzMeshContour DvzMeshContour;


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


struct DvzMeshLight {
    mat4 pos;        // w=0 indicates it's a direction with no position.
    mat4 color;      // alpha value indicates it's on.
};

struct DvzMeshMaterial {
    mat4 params;          /* (r, g, b, -) X (ambient, specular, diffuse, emission) */
    float shine;          /* specular amount */
    float emit;           /* emission level */
};

struct DvzMeshContour {
    vec4 edgecolor;      /* r, g, b, a */
    float linewidth;     /* contour line width */
    int isoline_count;   /* number of isolines */
};


typedef enum
{
    DVZ_LIGHT_PARAMS_POS,
    DVZ_LIGHT_PARAMS_COLOR,
} DvzLightParamsEnum;


typedef enum
{
    DVZ_MESH_PARAMS_PARAMS,
    DVZ_MESH_PARAMS_SHINE,
    DVZ_MESH_PARAMS_EMIT,
} DvzMeshParamsEnum;


typedef enum
{
    DVZ_MESH_PARAMS_EDGECOLOR,
    DVZ_MESH_PARAMS_LINEWIDTH,
    DVZ_MESH_PARAMS_ISOLINE_COUNT,
} DvzMeshContourEnum;


#endif
