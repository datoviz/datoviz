/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Geometry types                                                                               */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "datoviz/math/types.h"
#include "enums.h"
#include <inttypes.h>
#include <stdbool.h>



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzGeometry DvzGeometry;
typedef struct DvzGeometryBounds DvzGeometryBounds;
typedef struct DvzGeometryCubeDesc DvzGeometryCubeDesc;
typedef struct DvzGeometryPlaneDesc DvzGeometryPlaneDesc;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzGeometry
{
    DvzGeometryType type; // geometry source/type
    uint32_t flags;      // DvzGeometryIndexingFlags and future metadata flags

    uint32_t vertex_count; // number of vertices
    uint32_t index_count;  // number of triangle-list indices

    dvec3* positions;  // F64 3D positions
    dvec3* normals;    // F64 3D normal vectors
    DvzColor* colors;  // RGBA color of each vertex
    dvec2* texcoords;  // F64 texture coordinates
    DvzIndex* indices; // triangle-list index buffer
};



struct DvzGeometryBounds
{
    double xmin, xmax, ymin, ymax, zmin, zmax;
};



struct DvzGeometryCubeDesc
{
    dvec3 center;   // cube center
    double size;    // edge length
    DvzColor color; // vertex color, defaults to opaque white when all channels are zero
};



struct DvzGeometryPlaneDesc
{
    dvec3 center;   // plane center
    double width;   // plane width along X
    double height;  // plane height along Y
    double z;       // plane Z coordinate, added to center.z
    DvzColor color; // vertex color, defaults to opaque white when all channels are zero
};
