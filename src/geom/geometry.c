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
#include "datoviz/math/vec.h"

#include <float.h>
#include <math.h>
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
#define DVZ_GEOM_SPHERE_DEFAULT_RINGS 16
#define DVZ_GEOM_SPHERE_DEFAULT_SECTORS 32



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



/**
 * Return the squared length of a F64 3-vector.
 *
 * @param v vector
 * @return squared vector length
 */
static double _geom_dvec3_norm2(const dvec3 v)
{
    ANN(v);
    return v[0] * v[0] + v[1] * v[1] + v[2] * v[2];
}



/**
 * Return whether a vector is effectively zero.
 *
 * @param v vector
 * @return whether all components are near zero
 */
static bool _geom_dvec3_is_zero(const dvec3 v)
{
    ANN(v);
    return _geom_dvec3_norm2(v) <= EPSILON * EPSILON;
}



/**
 * Normalize a F64 3-vector in place.
 *
 * @param v vector
 * @return whether the vector was normalized
 */
static bool _geom_dvec3_normalize(dvec3 v)
{
    ANN(v);
    const double norm2 = _geom_dvec3_norm2(v);
    if (norm2 <= EPSILON * EPSILON || !isfinite(norm2))
        return false;

    const double inv_norm = 1.0 / sqrt(norm2);
    v[0] *= inv_norm;
    v[1] *= inv_norm;
    v[2] *= inv_norm;
    return true;
}



/**
 * Compute the cross product of two F64 3-vectors.
 *
 * @param a first vector
 * @param b second vector
 * @param out output vector
 */
static void _geom_dvec3_cross(const dvec3 a, const dvec3 b, dvec3 out)
{
    ANN(a);
    ANN(b);
    ANN(out);
    out[0] = a[1] * b[2] - a[2] * b[1];
    out[1] = a[2] * b[0] - a[0] * b[2];
    out[2] = a[0] * b[1] - a[1] * b[0];
}



/**
 * Compute one triangle normal from indexed positions.
 *
 * @param geometry the geometry
 * @param i0 first vertex index
 * @param i1 second vertex index
 * @param i2 third vertex index
 * @param out output normal
 * @return whether the normal is valid
 */
static bool _geom_triangle_normal(
    const DvzGeometry* geometry, DvzIndex i0, DvzIndex i1, DvzIndex i2, dvec3 out)
{
    ANN(geometry);
    ANN(out);
    ASSERT(i0 < geometry->vertex_count);
    ASSERT(i1 < geometry->vertex_count);
    ASSERT(i2 < geometry->vertex_count);

    const dvec3* p0 = &geometry->positions[i0];
    const dvec3* p1 = &geometry->positions[i1];
    const dvec3* p2 = &geometry->positions[i2];
    dvec3 a = {(*p1)[0] - (*p0)[0], (*p1)[1] - (*p0)[1], (*p1)[2] - (*p0)[2]};
    dvec3 b = {(*p2)[0] - (*p0)[0], (*p2)[1] - (*p0)[1], (*p2)[2] - (*p0)[2]};
    _geom_dvec3_cross(a, b, out);
    return _geom_dvec3_normalize(out);
}



/**
 * Validate and compute structured surface-grid counts.
 *
 * @param rows row count
 * @param cols column count
 * @param out_vertices output vertex count
 * @param out_indices output index count
 * @return whether the counts are valid
 */
static bool _geom_surface_grid_counts(
    uint32_t rows, uint32_t cols, uint32_t* out_vertices, uint32_t* out_indices)
{
    ANN(out_vertices);
    ANN(out_indices);
    if (rows < 2 || cols < 2)
        return false;

    uint64_t vertex_count = 0;
    if (_dvz_mul_u64_overflows((uint64_t)rows, (uint64_t)cols, &vertex_count) ||
        vertex_count > UINT32_MAX)
    {
        return false;
    }

    uint64_t quad_count = 0;
    if (_dvz_mul_u64_overflows((uint64_t)(rows - 1), (uint64_t)(cols - 1), &quad_count))
        return false;

    uint64_t index_count = 0;
    if (_dvz_mul_u64_overflows(quad_count, 6, &index_count) || index_count > UINT32_MAX)
        return false;

    *out_vertices = (uint32_t)vertex_count;
    *out_indices = (uint32_t)index_count;
    return true;
}



/**
 * Validate and compute UV-sphere counts.
 *
 * @param rings latitude segment count
 * @param sectors longitude segment count
 * @param out_vertices output vertex count
 * @param out_indices output index count
 * @return whether the counts are valid
 */
static bool _geom_sphere_counts(
    uint32_t rings, uint32_t sectors, uint32_t* out_vertices, uint32_t* out_indices)
{
    ANN(out_vertices);
    ANN(out_indices);
    if (rings < 2 || sectors < 3)
        return false;

    uint64_t rows = (uint64_t)rings + 1;
    uint64_t cols = (uint64_t)sectors + 1;
    uint64_t vertex_count = 0;
    if (_dvz_mul_u64_overflows(rows, cols, &vertex_count) || vertex_count > UINT32_MAX)
        return false;

    uint64_t quad_count = 0;
    if (_dvz_mul_u64_overflows((uint64_t)rings, (uint64_t)sectors, &quad_count))
        return false;

    uint64_t index_count = 0;
    if (_dvz_mul_u64_overflows(quad_count, 6, &index_count) || index_count > UINT32_MAX)
        return false;

    *out_vertices = (uint32_t)vertex_count;
    *out_indices = (uint32_t)index_count;
    return true;
}



/**
 * Fill surface-grid descriptor defaults.
 *
 * @param desc optional source descriptor
 * @param out output descriptor
 * @param color output fallback color
 */
static void _geom_surface_grid_config(
    const DvzGeometrySurfaceGridDesc* desc, DvzGeometrySurfaceGridDesc* out, DvzColor color)
{
    ANN(out);
    ANN(color);
    dvz_memset(out, sizeof(DvzGeometrySurfaceGridDesc), 0, sizeof(DvzGeometrySurfaceGridDesc));
    out->height_scale = 1.0;
    out->col_basis[0] = 1.0;
    out->row_basis[1] = 1.0;
    out->height_axis[2] = 1.0;
    if (desc != NULL)
        *out = *desc;

    if (_geom_dvec3_is_zero(out->col_basis))
    {
        out->col_basis[0] = 1.0;
        out->col_basis[1] = 0.0;
        out->col_basis[2] = 0.0;
    }
    if (_geom_dvec3_is_zero(out->row_basis))
    {
        out->row_basis[0] = 0.0;
        out->row_basis[1] = 1.0;
        out->row_basis[2] = 0.0;
    }
    if (_geom_dvec3_is_zero(out->height_axis))
    {
        out->height_axis[0] = 0.0;
        out->height_axis[1] = 0.0;
        out->height_axis[2] = 1.0;
    }
    if (out->height_scale == 0.0)
        out->height_scale = 1.0;

    _geom_color_or_default(out->color, color);
}



/**
 * Return whether a geometry can be consumed as a complete indexed payload.
 *
 * @param geometry the geometry
 * @return whether the geometry is valid
 */
static bool _geom_valid_payload(const DvzGeometry* geometry)
{
    return geometry != NULL && geometry->vertex_count > 0 && geometry->positions != NULL &&
           geometry->colors != NULL && geometry->texcoords != NULL &&
           (geometry->index_count == 0 || geometry->indices != NULL);
}



/**
 * Store structured-grid provenance on a geometry.
 *
 * @param geometry the geometry
 * @param cfg surface-grid descriptor
 */
static void _geom_surface_grid_store_provenance(
    DvzGeometry* geometry, const DvzGeometrySurfaceGridDesc* cfg)
{
    ANN(geometry);
    ANN(cfg);
    geometry->grid_rows = cfg->rows;
    geometry->grid_cols = cfg->cols;
    dvz_memcpy(geometry->grid_origin, sizeof(dvec3), cfg->origin, sizeof(dvec3));
    dvz_memcpy(geometry->grid_row_basis, sizeof(dvec3), cfg->row_basis, sizeof(dvec3));
    dvz_memcpy(geometry->grid_col_basis, sizeof(dvec3), cfg->col_basis, sizeof(dvec3));
    dvz_memcpy(geometry->grid_height_axis, sizeof(dvec3), cfg->height_axis, sizeof(dvec3));
    geometry->grid_height_scale = cfg->height_scale;
}



/**
 * Update one structured-grid vertex position from provenance and height.
 *
 * @param geometry the geometry
 * @param row row index
 * @param col column index
 * @param height height value
 */
static void _geom_surface_grid_update_position(
    DvzGeometry* geometry, uint32_t row, uint32_t col, double height)
{
    ANN(geometry);
    ASSERT(row < geometry->grid_rows);
    ASSERT(col < geometry->grid_cols);
    const uint32_t index = row * geometry->grid_cols + col;
    const double scaled_height = height * geometry->grid_height_scale;
    geometry->positions[index][0] =
        geometry->grid_origin[0] + (double)col * geometry->grid_col_basis[0] +
        (double)row * geometry->grid_row_basis[0] + scaled_height * geometry->grid_height_axis[0];
    geometry->positions[index][1] =
        geometry->grid_origin[1] + (double)col * geometry->grid_col_basis[1] +
        (double)row * geometry->grid_row_basis[1] + scaled_height * geometry->grid_height_axis[1];
    geometry->positions[index][2] =
        geometry->grid_origin[2] + (double)col * geometry->grid_col_basis[2] +
        (double)row * geometry->grid_row_basis[2] + scaled_height * geometry->grid_height_axis[2];
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
 * Recompute smooth vertex normals from triangle indices.
 *
 * @param geometry the geometry
 * @return 0 on success, -1 on invalid input
 */
int dvz_geometry_compute_normals(DvzGeometry* geometry)
{
    if (geometry == NULL || geometry->positions == NULL || geometry->normals == NULL ||
        geometry->indices == NULL || geometry->vertex_count == 0 || geometry->index_count == 0 ||
        geometry->index_count % 3 != 0)
    {
        return -1;
    }

    dvz_memset(geometry->normals, geometry->vertex_count * sizeof(dvec3), 0,
               geometry->vertex_count * sizeof(dvec3));

    for (uint32_t i = 0; i < geometry->index_count; i += 3)
    {
        const DvzIndex i0 = geometry->indices[i + 0];
        const DvzIndex i1 = geometry->indices[i + 1];
        const DvzIndex i2 = geometry->indices[i + 2];
        if (i0 >= geometry->vertex_count || i1 >= geometry->vertex_count ||
            i2 >= geometry->vertex_count)
        {
            return -1;
        }

        dvec3 n = {0};
        if (!_geom_triangle_normal(geometry, i0, i1, i2, n))
            continue;

        for (uint32_t j = 0; j < 3; j++)
        {
            geometry->normals[i0][j] += n[j];
            geometry->normals[i1][j] += n[j];
            geometry->normals[i2][j] += n[j];
        }
    }

    for (uint32_t i = 0; i < geometry->vertex_count; i++)
    {
        if (!_geom_dvec3_normalize(geometry->normals[i]))
        {
            geometry->normals[i][0] = 0.0;
            geometry->normals[i][1] = 0.0;
            geometry->normals[i][2] = 1.0;
        }
    }
    return 0;
}



/**
 * Apply an affine transform to positions and normals in place.
 *
 * @param geometry the geometry
 * @param transform affine transform matrix
 * @return 0 on success, -1 on invalid input
 */
int dvz_geometry_transform(DvzGeometry* geometry, dmat4 transform)
{
    if (geometry == NULL || transform == NULL || geometry->positions == NULL ||
        geometry->vertex_count == 0)
    {
        return -1;
    }

    const double a00 = transform[0][0], a01 = transform[1][0], a02 = transform[2][0];
    const double a10 = transform[0][1], a11 = transform[1][1], a12 = transform[2][1];
    const double a20 = transform[0][2], a21 = transform[1][2], a22 = transform[2][2];
    const double det = a00 * (a11 * a22 - a12 * a21) -
                       a01 * (a10 * a22 - a12 * a20) +
                       a02 * (a10 * a21 - a11 * a20);
    if (geometry->normals != NULL && fabs(det) <= EPSILON)
        return -1;

    const double inv_det = det != 0.0 ? 1.0 / det : 0.0;
    const double n00 = (a11 * a22 - a12 * a21) * inv_det;
    const double n01 = (a12 * a20 - a10 * a22) * inv_det;
    const double n02 = (a10 * a21 - a11 * a20) * inv_det;
    const double n10 = (a02 * a21 - a01 * a22) * inv_det;
    const double n11 = (a00 * a22 - a02 * a20) * inv_det;
    const double n12 = (a01 * a20 - a00 * a21) * inv_det;
    const double n20 = (a01 * a12 - a02 * a11) * inv_det;
    const double n21 = (a02 * a10 - a00 * a12) * inv_det;
    const double n22 = (a00 * a11 - a01 * a10) * inv_det;

    for (uint32_t i = 0; i < geometry->vertex_count; i++)
        dvz_dmat4_mulv3(transform, geometry->positions[i], 1.0, geometry->positions[i]);

    if (geometry->normals != NULL)
    {
        for (uint32_t i = 0; i < geometry->vertex_count; i++)
        {
            const double x = geometry->normals[i][0];
            const double y = geometry->normals[i][1];
            const double z = geometry->normals[i][2];
            geometry->normals[i][0] = n00 * x + n01 * y + n02 * z;
            geometry->normals[i][1] = n10 * x + n11 * y + n12 * z;
            geometry->normals[i][2] = n20 * x + n21 * y + n22 * z;
            _geom_dvec3_normalize(geometry->normals[i]);
        }
    }

    return 0;
}



/**
 * Merge several geometry objects into one indexed geometry.
 *
 * @param count number of input geometries
 * @param geometries input geometry array
 * @return the merged geometry, or NULL on failure
 */
DvzGeometry* dvz_geometry_merge(uint32_t count, const DvzGeometry* const* geometries)
{
    if (count == 0 || geometries == NULL)
        return NULL;

    uint64_t vertex_count = 0;
    uint64_t index_count = 0;
    for (uint32_t i = 0; i < count; i++)
    {
        const DvzGeometry* geometry = geometries[i];
        if (!_geom_valid_payload(geometry))
            return NULL;

        if (_dvz_add_u64_overflows(vertex_count, geometry->vertex_count, &vertex_count) ||
            _dvz_add_u64_overflows(index_count, geometry->index_count, &index_count) ||
            vertex_count > UINT32_MAX || index_count > UINT32_MAX)
        {
            return NULL;
        }
    }

    DvzGeometry* out = dvz_geometry((uint32_t)vertex_count, (uint32_t)index_count);
    if (out == NULL)
        return NULL;

    out->type = DVZ_GEOMETRY_CUSTOM;
    out->flags = DVZ_GEOMETRY_INDEXING_TRIANGLES;

    uint32_t vertex_offset = 0;
    uint32_t index_offset = 0;
    for (uint32_t i = 0; i < count; i++)
    {
        const DvzGeometry* geometry = geometries[i];
        const uint64_t position_size = (uint64_t)geometry->vertex_count * sizeof(dvec3);
        const uint64_t color_size = (uint64_t)geometry->vertex_count * sizeof(DvzColor);
        const uint64_t texcoord_size = (uint64_t)geometry->vertex_count * sizeof(dvec2);

        dvz_memcpy(&out->positions[vertex_offset], position_size, geometry->positions, position_size);
        if (geometry->normals != NULL)
            dvz_memcpy(&out->normals[vertex_offset], position_size, geometry->normals, position_size);
        if (geometry->colors != NULL)
            dvz_memcpy(&out->colors[vertex_offset], color_size, geometry->colors, color_size);
        if (geometry->texcoords != NULL)
            dvz_memcpy(
                &out->texcoords[vertex_offset], texcoord_size, geometry->texcoords, texcoord_size);

        for (uint32_t j = 0; j < geometry->index_count; j++)
        {
            const DvzIndex src_index = geometry->indices[j];
            if (src_index >= geometry->vertex_count)
            {
                dvz_geometry_destroy(out);
                return NULL;
            }
            out->indices[index_offset + j] = vertex_offset + src_index;
        }

        vertex_offset += geometry->vertex_count;
        index_offset += geometry->index_count;
    }

    return out;
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



/**
 * Create an indexed UV-sphere geometry.
 *
 * @param desc optional sphere descriptor
 * @return the new geometry, or NULL on failure
 */
DvzGeometry* dvz_geom_sphere(const DvzGeometrySphereDesc* desc)
{
    DvzGeometrySphereDesc cfg = {0};
    cfg.radius = 1.0;
    cfg.rings = DVZ_GEOM_SPHERE_DEFAULT_RINGS;
    cfg.sectors = DVZ_GEOM_SPHERE_DEFAULT_SECTORS;
    if (desc != NULL)
        cfg = *desc;
    if (cfg.radius <= 0.0)
        return NULL;
    if (cfg.rings == 0)
        cfg.rings = DVZ_GEOM_SPHERE_DEFAULT_RINGS;
    if (cfg.sectors == 0)
        cfg.sectors = DVZ_GEOM_SPHERE_DEFAULT_SECTORS;

    uint32_t vertex_count = 0;
    uint32_t index_count = 0;
    if (!_geom_sphere_counts(cfg.rings, cfg.sectors, &vertex_count, &index_count))
        return NULL;

    DvzColor color = {0};
    _geom_color_or_default(cfg.color, color);

    DvzGeometry* geometry = dvz_geometry(vertex_count, index_count);
    if (geometry == NULL)
        return NULL;

    geometry->type = DVZ_GEOMETRY_SPHERE;
    geometry->flags = DVZ_GEOMETRY_INDEXING_TRIANGLES;

    for (uint32_t ring = 0; ring <= cfg.rings; ring++)
    {
        const double v = (double)ring / (double)cfg.rings;
        const double theta = M_PI * v;
        const double st = sin(theta);
        const double ct = cos(theta);
        for (uint32_t sector = 0; sector <= cfg.sectors; sector++)
        {
            const double u = (double)sector / (double)cfg.sectors;
            const double phi = M_2PI * u;
            const double cp = cos(phi);
            const double sp = sin(phi);
            const dvec3 normal = {st * cp, st * sp, ct};
            const dvec3 position = {
                cfg.center[0] + cfg.radius * normal[0],
                cfg.center[1] + cfg.radius * normal[1],
                cfg.center[2] + cfg.radius * normal[2],
            };
            const dvec2 texcoord = {u, v};
            _geom_set_vertex(
                geometry, ring * (cfg.sectors + 1) + sector, position, normal, texcoord, color);
        }
    }

    uint32_t index_offset = 0;
    const uint32_t cols = cfg.sectors + 1;
    for (uint32_t ring = 0; ring < cfg.rings; ring++)
    {
        for (uint32_t sector = 0; sector < cfg.sectors; sector++)
        {
            const uint32_t i0 = ring * cols + sector;
            const uint32_t i1 = i0 + 1;
            const uint32_t i2 = i0 + cols;
            const uint32_t i3 = i2 + 1;
            _geom_set_index(geometry, index_offset + 0, i0);
            _geom_set_index(geometry, index_offset + 1, i3);
            _geom_set_index(geometry, index_offset + 2, i1);
            _geom_set_index(geometry, index_offset + 3, i0);
            _geom_set_index(geometry, index_offset + 4, i2);
            _geom_set_index(geometry, index_offset + 5, i3);
            index_offset += 6;
        }
    }

    return geometry;
}



/**
 * Create an indexed structured surface-grid geometry.
 *
 * @param desc surface-grid descriptor
 * @return the new geometry, or NULL on failure
 */
DvzGeometry* dvz_geom_surface_grid(const DvzGeometrySurfaceGridDesc* desc)
{
    if (desc == NULL)
        return NULL;

    DvzGeometrySurfaceGridDesc cfg = {0};
    DvzColor fallback_color = {0};
    _geom_surface_grid_config(desc, &cfg, fallback_color);

    uint32_t vertex_count = 0;
    uint32_t index_count = 0;
    if (!_geom_surface_grid_counts(cfg.rows, cfg.cols, &vertex_count, &index_count))
        return NULL;

    DvzGeometry* geometry = dvz_geometry(vertex_count, index_count);
    if (geometry == NULL)
        return NULL;

    geometry->type = DVZ_GEOMETRY_SURFACE_GRID;
    geometry->flags = DVZ_GEOMETRY_INDEXING_TRIANGLES | DVZ_GEOMETRY_INDEXING_SURFACE_GRID;
    _geom_surface_grid_store_provenance(geometry, &cfg);

    for (uint32_t row = 0; row < cfg.rows; row++)
    {
        const double v = cfg.rows > 1 ? (double)row / (double)(cfg.rows - 1) : 0.0;
        for (uint32_t col = 0; col < cfg.cols; col++)
        {
            const uint32_t index = row * cfg.cols + col;
            const double u = cfg.cols > 1 ? (double)col / (double)(cfg.cols - 1) : 0.0;
            const double height = cfg.heights != NULL ? cfg.heights[index] : 0.0;
            _geom_surface_grid_update_position(geometry, row, col, height);
            const dvec3 normal = {0.0, 0.0, 1.0};
            const dvec2 texcoord = {u, v};
            const DvzColor* color = cfg.colors != NULL ? &cfg.colors[index] : &fallback_color;
            _geom_set_vertex(
                geometry, index, geometry->positions[index], normal, texcoord, *color);
        }
    }

    uint32_t index_offset = 0;
    for (uint32_t row = 0; row < cfg.rows - 1; row++)
    {
        for (uint32_t col = 0; col < cfg.cols - 1; col++)
        {
            const uint32_t i0 = row * cfg.cols + col;
            const uint32_t i1 = i0 + 1;
            const uint32_t i2 = i0 + cfg.cols;
            const uint32_t i3 = i2 + 1;
            _geom_set_index(geometry, index_offset + 0, i0);
            _geom_set_index(geometry, index_offset + 1, i1);
            _geom_set_index(geometry, index_offset + 2, i3);
            _geom_set_index(geometry, index_offset + 3, i0);
            _geom_set_index(geometry, index_offset + 4, i3);
            _geom_set_index(geometry, index_offset + 5, i2);
            index_offset += 6;
        }
    }

    if (dvz_geometry_compute_normals(geometry) != 0)
    {
        dvz_geometry_destroy(geometry);
        return NULL;
    }

    return geometry;
}



/**
 * Update the heights of an existing structured surface-grid geometry.
 *
 * @param geometry the surface-grid geometry
 * @param heights row-major height values
 * @param count number of height values
 * @return 0 on success, -1 on invalid input
 */
int dvz_geom_surface_grid_update_heights(
    DvzGeometry* geometry, const double* heights, uint32_t count)
{
    if (geometry == NULL || heights == NULL || geometry->type != DVZ_GEOMETRY_SURFACE_GRID ||
        geometry->positions == NULL || geometry->grid_rows < 2 || geometry->grid_cols < 2 ||
        count != geometry->vertex_count || geometry->vertex_count == 0)
    {
        return -1;
    }

    uint64_t expected = 0;
    if (_dvz_mul_u64_overflows(
            (uint64_t)geometry->grid_rows, (uint64_t)geometry->grid_cols, &expected) ||
        expected != geometry->vertex_count)
    {
        return -1;
    }

    for (uint32_t row = 0; row < geometry->grid_rows; row++)
    {
        for (uint32_t col = 0; col < geometry->grid_cols; col++)
        {
            const uint32_t index = row * geometry->grid_cols + col;
            _geom_surface_grid_update_position(geometry, row, col, heights[index]);
        }
    }

    return dvz_geometry_compute_normals(geometry);
}
