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
typedef struct DvzGeometryDiscDesc DvzGeometryDiscDesc;
typedef struct DvzGeometrySectorDesc DvzGeometrySectorDesc;
typedef struct DvzGeometryRegularPolygonDesc DvzGeometryRegularPolygonDesc;
typedef struct DvzGeometryStarDesc DvzGeometryStarDesc;
typedef struct DvzGeometryCylinderDesc DvzGeometryCylinderDesc;
typedef struct DvzGeometryConeDesc DvzGeometryConeDesc;
typedef struct DvzGeometryTorusDesc DvzGeometryTorusDesc;
typedef struct DvzGeometryArrowDesc DvzGeometryArrowDesc;
typedef struct DvzGeometryObjDesc DvzGeometryObjDesc;
typedef struct DvzPolygonRing DvzPolygonRing;
typedef struct DvzPolygonDesc DvzPolygonDesc;
typedef struct DvzTriangulationDesc DvzTriangulationDesc;
typedef struct DvzBezierTessellationDesc DvzBezierTessellationDesc;
typedef struct DvzTessellatedPath DvzTessellatedPath;
typedef struct DvzGeometryEdge DvzGeometryEdge;
typedef struct DvzGeometryEdges DvzGeometryEdges;
typedef struct DvzGeometryContourSegment DvzGeometryContourSegment;
typedef struct DvzGeometryContours DvzGeometryContours;



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

    /*
     * Owned arrays released by dvz_geometry_destroy(). Callers may edit element contents but must
     * not free, reallocate, replace these pointers, or desynchronize them from the count fields.
     */
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
    uint32_t struct_size;
    uint32_t flags;
    dvec3 center;                 // cube center
    double size;                  // edge length
    DvzColor color;               // fallback vertex color, defaults to opaque white
    const DvzColor* face_colors;  // optional six face colors: +X, -X, +Y, -Y, +Z, -Z
    uint32_t face_color_count;    // number of entries in face_colors, must be at least 6
};



struct DvzGeometryPlaneDesc
{
    uint32_t struct_size;
    uint32_t flags;
    dvec3 center;   // plane center
    double width;   // plane width along X
    double height;  // plane height along Y
    double z;       // plane Z coordinate, added to center.z
    DvzColor color; // vertex color, defaults to opaque white when all channels are zero
};



struct DvzGeometrySphereDesc
{
    uint32_t struct_size;
    uint32_t flags;
    dvec3 center;   // sphere center
    double radius;  // sphere radius
    uint32_t rings; // latitude segment count, defaults to 16
    uint32_t sectors; // longitude segment count, defaults to 32
    DvzColor color; // vertex color, defaults to opaque white when all channels are zero
};



struct DvzGeometrySurfaceGridDesc
{
    uint32_t struct_size;
    uint32_t flags;
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



struct DvzGeometryDiscDesc
{
    uint32_t struct_size;
    uint32_t flags;
    dvec3 center;      // disc center
    double radius;     // disc radius
    uint32_t segments; // perimeter segment count, defaults to 48
    DvzColor color;    // vertex color, defaults to opaque white when all channels are zero
};



struct DvzGeometrySectorDesc
{
    uint32_t struct_size;
    uint32_t flags;
    dvec3 center;       // sector center
    double radius;      // sector radius
    double start_angle; // start angle in radians
    double sweep_angle; // angular span in radians, defaults to 90 degrees
    uint32_t segments;  // arc segment count, defaults to 32
    DvzColor color;     // vertex color, defaults to opaque white when all channels are zero
};



struct DvzGeometryRegularPolygonDesc
{
    uint32_t struct_size;
    uint32_t flags;
    dvec3 center;   // polygon center
    double radius;  // circumradius
    uint32_t sides; // side count, defaults to 6
    DvzColor color; // vertex color, defaults to opaque white when all channels are zero
};



struct DvzGeometryStarDesc
{
    uint32_t struct_size;
    uint32_t flags;
    dvec3 center;        // star center
    double outer_radius; // outer vertex radius
    double inner_radius; // inner vertex radius
    uint32_t points;     // number of star points, defaults to 5
    DvzColor color;      // vertex color, defaults to opaque white when all channels are zero
};



struct DvzGeometryCylinderDesc
{
    uint32_t struct_size;
    uint32_t flags;
    dvec3 center;      // cylinder center
    double radius;     // cylinder radius
    double height;     // cylinder height along Z
    uint32_t sectors;  // radial segment count, defaults to 32
    DvzColor color;    // vertex color, defaults to opaque white when all channels are zero
};



struct DvzGeometryConeDesc
{
    uint32_t struct_size;
    uint32_t flags;
    dvec3 center;     // cone center
    double radius;    // base radius
    double height;    // cone height along Z
    uint32_t sectors; // radial segment count, defaults to 32
    DvzColor color;   // vertex color, defaults to opaque white when all channels are zero
};



struct DvzGeometryTorusDesc
{
    uint32_t struct_size;
    uint32_t flags;
    dvec3 center;        // torus center
    double major_radius; // distance from center to tube center
    double minor_radius; // tube radius
    uint32_t rings;      // major-ring segment count, defaults to 32
    uint32_t sectors;    // tube segment count, defaults to 16
    DvzColor color;      // vertex color, defaults to opaque white when all channels are zero
};



struct DvzGeometryArrowDesc
{
    uint32_t struct_size;
    uint32_t flags;
    dvec3 center;        // arrow center
    double length;       // total length along Z
    double shaft_radius; // shaft cylinder radius
    double head_radius;  // cone head radius
    double head_length;  // cone head length
    uint32_t sectors;    // radial segment count, defaults to 32
    DvzColor color;      // vertex color, defaults to opaque white when all channels are zero
};



struct DvzGeometryObjDesc
{
    uint32_t struct_size;
    uint32_t flags;
    DvzColor color; // vertex color, defaults to opaque white when all channels are zero
};



struct DvzPolygonRing
{
    const dvec2* xy; // borrowed XY vertex array
    uint32_t count; // number of vertices, optionally including a repeated closing vertex
};



struct DvzPolygonDesc
{
    uint32_t struct_size;
    uint32_t flags;
    DvzPolygonRing outer;         // borrowed outer boundary ring
    const DvzPolygonRing* holes;  // borrowed hole ring array, or NULL when hole_count is zero
    uint32_t hole_count;          // number of hole rings
};



struct DvzTriangulationDesc
{
    uint32_t struct_size;
    uint32_t flags;
    DvzTriangulationBackend backend; // triangulation backend, defaults to Earcut
};



struct DvzBezierTessellationDesc
{
    uint32_t struct_size;
    uint32_t flags;
    uint32_t segment_count; // number of line segments, defaults to 32
    double tolerance;       // reserved for future adaptive tessellation, defaults to 0
};



struct DvzTessellatedPath
{
    uint32_t point_count; // number of sampled path points
    dvec3* points;       // owned F64 3D path points
};



struct DvzGeometryEdge
{
    DvzIndex v0; // first endpoint vertex index, canonicalized so v0 <= v1
    DvzIndex v1; // second endpoint vertex index

    uint32_t face0;          // first adjacent triangle index, or UINT32_MAX when absent
    uint32_t face1;          // second adjacent triangle index, or UINT32_MAX when absent
    uint32_t adjacent_count; // number of adjacent triangles
    uint32_t flags;          // DvzGeometryEdgeFlags
};



struct DvzGeometryEdges
{
    uint32_t edge_count;    // number of unique edges
    DvzGeometryEdge* edges; // edge array
};



struct DvzGeometryContourSegment
{
    dvec3 p0; // first contour segment endpoint
    dvec3 p1; // second contour segment endpoint

    double level;        // contour level value
    uint32_t level_index; // index in the input level array
    uint32_t face_index;  // source triangle index
};



struct DvzGeometryContours
{
    uint32_t segment_count;              // number of contour segments
    DvzGeometryContourSegment* segments; // contour segment array
};
