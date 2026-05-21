/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Geometry utilities                                                                           */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "datoviz/geom.h"

#include <float.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_compat.h"
#include "_log.h"
#include "_overflow.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define DVZ_GEOM_CUBE_VERTEX_COUNT 24
#define DVZ_GEOM_CUBE_INDEX_COUNT  36
#define DVZ_GEOM_PLANE_VERTEX_COUNT 4
#define DVZ_GEOM_PLANE_INDEX_COUNT  6



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Return whether a count-sized allocation is representable.
 *
 * @param count number of items
 * @param item_size item size in bytes
 * @return whether the allocation size is valid
 */
static bool _geom_allocation_valid(uint32_t count, DvzSize item_size)
{
    if (count == 0)
        return true;

    uint64_t total = 0;
    return !_dvz_mul_u64_overflows((uint64_t)count, (uint64_t)item_size, &total);
}



/**
 * Copy a color with opaque white fallback.
 *
 * @param color source color
 * @param out output color
 */
static void _geom_color_or_default(const DvzColor color, DvzColor out)
{
    ANN(out);
    if (color[0] == 0 && color[1] == 0 && color[2] == 0 && color[3] == 0)
    {
        out[0] = 255;
        out[1] = 255;
        out[2] = 255;
        out[3] = 255;
        return;
    }

    out[0] = color[0];
    out[1] = color[1];
    out[2] = color[2];
    out[3] = color[3];
}



/**
 * Assign one geometry vertex.
 *
 * @param geometry the geometry
 * @param index vertex index
 * @param position vertex position
 * @param normal vertex normal
 * @param texcoord vertex texture coordinate
 * @param color vertex color
 */
static void _geom_set_vertex(
    DvzGeometry* geometry, uint32_t index, const dvec3 position, const dvec3 normal,
    const dvec2 texcoord, const DvzColor color)
{
    ANN(geometry);
    ANN(position);
    ANN(normal);
    ANN(texcoord);
    ANN(color);
    ASSERT(index < geometry->vertex_count);

    geometry->positions[index][0] = position[0];
    geometry->positions[index][1] = position[1];
    geometry->positions[index][2] = position[2];

    geometry->normals[index][0] = normal[0];
    geometry->normals[index][1] = normal[1];
    geometry->normals[index][2] = normal[2];

    geometry->texcoords[index][0] = texcoord[0];
    geometry->texcoords[index][1] = texcoord[1];

    geometry->colors[index][0] = color[0];
    geometry->colors[index][1] = color[1];
    geometry->colors[index][2] = color[2];
    geometry->colors[index][3] = color[3];
}



/**
 * Assign one index value.
 *
 * @param geometry the geometry
 * @param index index-buffer offset
 * @param value index value
 */
static void _geom_set_index(DvzGeometry* geometry, uint32_t index, DvzIndex value)
{
    ANN(geometry);
    ASSERT(index < geometry->index_count);
    ASSERT(value < geometry->vertex_count);
    geometry->indices[index] = value;
}



/**
 * Fill all geometry vertices with one color.
 *
 * @param geometry the geometry
 * @param color vertex color
 */
static void _geom_fill_color(DvzGeometry* geometry, const DvzColor color)
{
    ANN(geometry);
    ANN(color);
    for (uint32_t i = 0; i < geometry->vertex_count; i++)
    {
        geometry->colors[i][0] = color[0];
        geometry->colors[i][1] = color[1];
        geometry->colors[i][2] = color[2];
        geometry->colors[i][3] = color[3];
    }
}



/**
 * Copy F64 vectors to F32 vectors.
 *
 * @param count vector count
 * @param src source F64 vectors
 * @param out output F32 vectors
 */
static void _geom_copy_dvec3_to_vec3(uint32_t count, const dvec3* src, vec3* out)
{
    ANN(src);
    ANN(out);
    for (uint32_t i = 0; i < count; i++)
    {
        out[i][0] = (float)src[i][0];
        out[i][1] = (float)src[i][1];
        out[i][2] = (float)src[i][2];
    }
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Allocate a geometry object with owned vertex and index buffers.
 *
 * @param vertex_count number of vertices
 * @param index_count number of indices
 * @return the new geometry, or NULL on failure
 */
DvzGeometry* dvz_geometry(uint32_t vertex_count, uint32_t index_count)
{
    if (!_geom_allocation_valid(vertex_count, sizeof(dvec3)) ||
        !_geom_allocation_valid(vertex_count, sizeof(DvzColor)) ||
        !_geom_allocation_valid(vertex_count, sizeof(dvec2)) ||
        !_geom_allocation_valid(index_count, sizeof(DvzIndex)))
    {
        log_error("geometry allocation size overflow");
        return NULL;
    }

    DvzGeometry* geometry = (DvzGeometry*)dvz_calloc(1, sizeof(DvzGeometry));
    if (geometry == NULL)
        return NULL;

    geometry->vertex_count = vertex_count;
    geometry->index_count = index_count;

    if (vertex_count > 0)
    {
        geometry->positions = (dvec3*)dvz_calloc(vertex_count, sizeof(dvec3));
        geometry->normals = (dvec3*)dvz_calloc(vertex_count, sizeof(dvec3));
        geometry->colors = (DvzColor*)dvz_calloc(vertex_count, sizeof(DvzColor));
        geometry->texcoords = (dvec2*)dvz_calloc(vertex_count, sizeof(dvec2));
        if (geometry->positions == NULL || geometry->normals == NULL || geometry->colors == NULL ||
            geometry->texcoords == NULL)
        {
            dvz_geometry_destroy(geometry);
            return NULL;
        }
    }

    if (index_count > 0)
    {
        geometry->indices = (DvzIndex*)dvz_calloc(index_count, sizeof(DvzIndex));
        if (geometry->indices == NULL)
        {
            dvz_geometry_destroy(geometry);
            return NULL;
        }
    }

    return geometry;
}



/**
 * Free all buffers owned by a geometry object and reset it to an empty state.
 *
 * @param geometry the geometry
 */
void dvz_geometry_reset(DvzGeometry* geometry)
{
    if (geometry == NULL)
        return;

    dvz_free(geometry->positions);
    dvz_free(geometry->normals);
    dvz_free(geometry->colors);
    dvz_free(geometry->texcoords);
    dvz_free(geometry->indices);

    dvz_memset(geometry, sizeof(DvzGeometry), 0, sizeof(DvzGeometry));
}



/**
 * Destroy a geometry object.
 *
 * @param geometry the geometry
 */
void dvz_geometry_destroy(DvzGeometry* geometry)
{
    if (geometry == NULL)
        return;

    dvz_geometry_reset(geometry);
    dvz_free(geometry);
}



/**
 * Compute the bounds of a geometry object's positions.
 *
 * @param geometry the geometry
 * @return the geometry bounds, or an empty zero bounds when no vertices exist
 */
DvzGeometryBounds dvz_geometry_bounds(const DvzGeometry* geometry)
{
    if (geometry == NULL || geometry->vertex_count == 0 || geometry->positions == NULL)
        return (DvzGeometryBounds){0};

    double xmin = DBL_MAX;
    double xmax = -DBL_MAX;
    double ymin = DBL_MAX;
    double ymax = -DBL_MAX;
    double zmin = DBL_MAX;
    double zmax = -DBL_MAX;

    for (uint32_t i = 0; i < geometry->vertex_count; i++)
    {
        const dvec3* p = &geometry->positions[i];
        xmin = (*p)[0] < xmin ? (*p)[0] : xmin;
        xmax = (*p)[0] > xmax ? (*p)[0] : xmax;
        ymin = (*p)[1] < ymin ? (*p)[1] : ymin;
        ymax = (*p)[1] > ymax ? (*p)[1] : ymax;
        zmin = (*p)[2] < zmin ? (*p)[2] : zmin;
        zmax = (*p)[2] > zmax ? (*p)[2] : zmax;
    }

    return (DvzGeometryBounds){
        .xmin = xmin, .xmax = xmax, .ymin = ymin, .ymax = ymax, .zmin = zmin, .zmax = zmax};
}



/**
 * Convert geometry positions to F32 vectors for current scene mesh upload paths.
 *
 * @param geometry the geometry
 * @param out output F32 position buffer
 * @param out_count number of vectors available in out
 * @return 0 on success, -1 on invalid input
 */
int dvz_geometry_positions_f32(const DvzGeometry* geometry, vec3* out, uint32_t out_count)
{
    if (geometry == NULL || out == NULL || geometry->positions == NULL ||
        out_count < geometry->vertex_count)
        return -1;

    _geom_copy_dvec3_to_vec3(geometry->vertex_count, geometry->positions, out);
    return 0;
}



/**
 * Convert geometry normals to F32 vectors for current scene mesh upload paths.
 *
 * @param geometry the geometry
 * @param out output F32 normal buffer
 * @param out_count number of vectors available in out
 * @return 0 on success, -1 on invalid input
 */
int dvz_geometry_normals_f32(const DvzGeometry* geometry, vec3* out, uint32_t out_count)
{
    if (geometry == NULL || out == NULL || geometry->normals == NULL ||
        out_count < geometry->vertex_count)
        return -1;

    _geom_copy_dvec3_to_vec3(geometry->vertex_count, geometry->normals, out);
    return 0;
}



/**
 * Create an indexed cube geometry.
 *
 * @param desc optional cube descriptor
 * @return the new geometry, or NULL on failure
 */
DvzGeometry* dvz_geom_cube(const DvzGeometryCubeDesc* desc)
{
    DvzGeometryCubeDesc cfg = {0};
    cfg.size = 1.0;
    if (desc != NULL)
        cfg = *desc;
    if (cfg.size <= 0)
        return NULL;

    DvzColor color = {0};
    _geom_color_or_default(cfg.color, color);

    DvzGeometry* geometry = dvz_geometry(DVZ_GEOM_CUBE_VERTEX_COUNT, DVZ_GEOM_CUBE_INDEX_COUNT);
    if (geometry == NULL)
        return NULL;

    geometry->type = DVZ_GEOMETRY_CUBE;
    geometry->flags = DVZ_GEOMETRY_INDEXING_TRIANGLES;

    const double h = cfg.size * 0.5;
    const double cx = cfg.center[0];
    const double cy = cfg.center[1];
    const double cz = cfg.center[2];
    const dvec2 uv[4] = {{0, 0}, {1, 0}, {1, 1}, {0, 1}};

    const dvec3 faces[6][4] = {
        {{cx + h, cy - h, cz - h}, {cx + h, cy + h, cz - h}, {cx + h, cy + h, cz + h},
         {cx + h, cy - h, cz + h}},
        {{cx - h, cy + h, cz - h}, {cx - h, cy - h, cz - h}, {cx - h, cy - h, cz + h},
         {cx - h, cy + h, cz + h}},
        {{cx - h, cy + h, cz - h}, {cx + h, cy + h, cz - h}, {cx + h, cy + h, cz + h},
         {cx - h, cy + h, cz + h}},
        {{cx + h, cy - h, cz - h}, {cx - h, cy - h, cz - h}, {cx - h, cy - h, cz + h},
         {cx + h, cy - h, cz + h}},
        {{cx - h, cy - h, cz + h}, {cx + h, cy - h, cz + h}, {cx + h, cy + h, cz + h},
         {cx - h, cy + h, cz + h}},
        {{cx - h, cy + h, cz - h}, {cx + h, cy + h, cz - h}, {cx + h, cy - h, cz - h},
         {cx - h, cy - h, cz - h}},
    };
    const dvec3 normals[6] = {
        {+1, 0, 0}, {-1, 0, 0}, {0, +1, 0}, {0, -1, 0}, {0, 0, +1}, {0, 0, -1}};

    for (uint32_t face = 0; face < 6; face++)
    {
        const uint32_t base = 4 * face;
        for (uint32_t j = 0; j < 4; j++)
            _geom_set_vertex(geometry, base + j, faces[face][j], normals[face], uv[j], color);

        const uint32_t index_base = 6 * face;
        _geom_set_index(geometry, index_base + 0, base + 0);
        _geom_set_index(geometry, index_base + 1, base + 1);
        _geom_set_index(geometry, index_base + 2, base + 2);
        _geom_set_index(geometry, index_base + 3, base + 0);
        _geom_set_index(geometry, index_base + 4, base + 2);
        _geom_set_index(geometry, index_base + 5, base + 3);
    }

    return geometry;
}



/**
 * Create an indexed XY plane geometry.
 *
 * @param desc optional plane descriptor
 * @return the new geometry, or NULL on failure
 */
DvzGeometry* dvz_geom_plane(const DvzGeometryPlaneDesc* desc)
{
    DvzGeometryPlaneDesc cfg = {0};
    cfg.width = 1.0;
    cfg.height = 1.0;
    if (desc != NULL)
        cfg = *desc;
    if (cfg.width <= 0 || cfg.height <= 0)
        return NULL;

    DvzColor color = {0};
    _geom_color_or_default(cfg.color, color);

    DvzGeometry* geometry = dvz_geometry(DVZ_GEOM_PLANE_VERTEX_COUNT, DVZ_GEOM_PLANE_INDEX_COUNT);
    if (geometry == NULL)
        return NULL;

    geometry->type = DVZ_GEOMETRY_PLANE;
    geometry->flags = DVZ_GEOMETRY_INDEXING_TRIANGLES;

    const double hx = cfg.width * 0.5;
    const double hy = cfg.height * 0.5;
    const double z = cfg.center[2] + cfg.z;
    const dvec3 normal = {0, 0, +1};
    const dvec3 positions[4] = {
        {cfg.center[0] - hx, cfg.center[1] - hy, z},
        {cfg.center[0] + hx, cfg.center[1] - hy, z},
        {cfg.center[0] + hx, cfg.center[1] + hy, z},
        {cfg.center[0] - hx, cfg.center[1] + hy, z},
    };
    const dvec2 uv[4] = {{0, 0}, {1, 0}, {1, 1}, {0, 1}};

    for (uint32_t i = 0; i < 4; i++)
        _geom_set_vertex(geometry, i, positions[i], normal, uv[i], color);
    _geom_fill_color(geometry, color);

    _geom_set_index(geometry, 0, 0);
    _geom_set_index(geometry, 1, 1);
    _geom_set_index(geometry, 2, 2);
    _geom_set_index(geometry, 3, 0);
    _geom_set_index(geometry, 4, 2);
    _geom_set_index(geometry, 5, 3);

    return geometry;
}
