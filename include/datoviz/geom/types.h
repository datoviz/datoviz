/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Common types                                                                                 */
/*************************************************************************************************/

#pragma once


/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <inttypes.h>
#include <stdbool.h>
#include <string.h>

#include "datoviz/color/types.h"
#include "datoviz/math.h"
#include "enums.h"



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzShape DvzShape;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzShape
{
    // Transform variables during transform begin/end.
    mat4 transform; // transformation matrix
    uint32_t first; // first vertex to transform
    uint32_t count; // number of vertices to transform

    DvzShapeType type;     // shape type
    uint32_t vertex_count; // number of vertices
    uint32_t index_count;  // number of indices (three times the number of triangle faces)

    vec3* pos;       // 3D positions of each vertex
    vec3* normal;    // 3D normal vector at each vertex
    DvzColor* color; // RGBA color of each vertex
    vec4* texcoords; // texture coordinates as u, v, (unused), alpha
    float* isoline;  // scalar field for isolines
    vec3* d_left;    // the distance of each vertex to the left edge adjacent to each face vertex
    vec3* d_right;   // the distance of each vertex to the right edge adjacent to each face vertex
    cvec4* contour;  // in each face, a bit mask with 1 if the opposite edge belongs to the contour
                     // edge, 2 if it is a corner, 4 if it should be oriented differently
    DvzIndex* index; // the index buffer

    // UGLY HACK: this seems to be necessary to ensure struct size equality between C and ctypes
    // (just checkstructs), maybe some alignment issue.
    // double _;
};
