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
typedef struct DvzGeometrySphereDesc DvzGeometrySphereDesc;
typedef struct DvzGeometrySurfaceGridDesc DvzGeometrySurfaceGridDesc;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzGeometry
{
    DvzGeometryType type; // geometry source/type
    uint32_t flags;      // DvzGeometryIndexingFlags and future metadata flags

    uint32_t vertex_count; // number of vertices
    uint32_t index_count;  // number of triangle-list indices

    uint32_t grid_rows; // row count for structured-grid provenance, or zero
    uint32_t grid_cols; // column count for structured-grid provenance, or zero
    dvec3 grid_origin;      // surface-grid origin
    dvec3 grid_row_basis;   // surface-grid row step vector
    dvec3 grid_col_basis;   // surface-grid column step vector
    dvec3 grid_height_axis; // surface-grid height displacement axis
    double grid_height_scale; // surface-grid height multiplier

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
    dvec3 center;                 // cube center
    double size;                  // edge length
    DvzColor color;               // fallback vertex color, defaults to opaque white
    const DvzColor* face_colors;  // optional six face colors: +X, -X, +Y, -Y, +Z, -Z
    uint32_t face_color_count;    // number of entries in face_colors, must be at least 6
};



struct DvzGeometryPlaneDesc
{
    dvec3 center;   // plane center
    double width;   // plane width along X
    double height;  // plane height along Y
    double z;       // plane Z coordinate, added to center.z
    DvzColor color; // vertex color, defaults to opaque white when all channels are zero
};



struct DvzGeometrySphereDesc
{
    dvec3 center;   // sphere center
    double radius;  // sphere radius
    uint32_t rings; // latitude segment count, defaults to 16
    uint32_t sectors; // longitude segment count, defaults to 32
    DvzColor color; // vertex color, defaults to opaque white when all channels are zero
};



struct DvzGeometrySurfaceGridDesc
{
    uint32_t rows; // number of grid rows
    uint32_t cols; // number of grid columns

    const double* heights;  // optional row-major height values
    const DvzColor* colors; // optional row-major vertex colors

    dvec3 origin;      // position of row 0, column 0
    dvec3 row_basis;   // step vector when row increases, defaults to +Y
    dvec3 col_basis;   // step vector when column increases, defaults to +X
    dvec3 height_axis; // height displacement axis, defaults to +Z

    double height_scale; // multiplier applied to heights, defaults to 1
    DvzColor color;      // fallback vertex color, defaults to opaque white
};
