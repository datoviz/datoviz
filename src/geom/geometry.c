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
#include "datoviz/ffi.h"
#include "datoviz/math/vec.h"

#include <float.h>
#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

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
#define DVZ_GEOM_DISC_DEFAULT_SEGMENTS 48
#define DVZ_GEOM_SECTOR_DEFAULT_SEGMENTS 32
#define DVZ_GEOM_POLYGON_DEFAULT_SIDES 6
#define DVZ_GEOM_STAR_DEFAULT_POINTS 5
#define DVZ_GEOM_REVOLUTION_DEFAULT_SECTORS 32
#define DVZ_GEOM_TORUS_DEFAULT_RINGS 32
#define DVZ_GEOM_TORUS_DEFAULT_SECTORS 16
#define DVZ_GEOMETRY_CUBE_DESC_KNOWN_FLAGS 0u
#define DVZ_GEOMETRY_PLANE_DESC_KNOWN_FLAGS 0u
#define DVZ_GEOMETRY_SPHERE_DESC_KNOWN_FLAGS 0u
#define DVZ_GEOMETRY_SURFACE_GRID_DESC_KNOWN_FLAGS 0u
#define DVZ_GEOMETRY_DISC_DESC_KNOWN_FLAGS 0u
#define DVZ_GEOMETRY_SECTOR_DESC_KNOWN_FLAGS 0u
#define DVZ_GEOMETRY_REGULAR_POLYGON_DESC_KNOWN_FLAGS 0u
#define DVZ_GEOMETRY_STAR_DESC_KNOWN_FLAGS 0u
#define DVZ_GEOMETRY_CYLINDER_DESC_KNOWN_FLAGS 0u
#define DVZ_GEOMETRY_CONE_DESC_KNOWN_FLAGS 0u
#define DVZ_GEOMETRY_TORUS_DESC_KNOWN_FLAGS 0u
#define DVZ_GEOMETRY_ARROW_DESC_KNOWN_FLAGS 0u



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct _GeomEdgeCandidate
{
    DvzIndex v0;
    DvzIndex v1;
    uint32_t face;
};



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

static bool _geometry_cube_desc_validate(const DvzGeometryCubeDesc* desc)
{
    if (desc == NULL)
        return true;
    if (!DVZ_STRUCT_VALID(desc, DvzGeometryCubeDesc, DVZ_GEOMETRY_CUBE_DESC_KNOWN_FLAGS))
    {
        log_error("invalid DvzGeometryCubeDesc ABI prologue");
        return false;
    }
    return true;
}



static bool _geometry_plane_desc_validate(const DvzGeometryPlaneDesc* desc)
{
    if (desc == NULL)
        return true;
    if (!DVZ_STRUCT_VALID(desc, DvzGeometryPlaneDesc, DVZ_GEOMETRY_PLANE_DESC_KNOWN_FLAGS))
    {
        log_error("invalid DvzGeometryPlaneDesc ABI prologue");
        return false;
    }
    return true;
}



static bool _geometry_sphere_desc_validate(const DvzGeometrySphereDesc* desc)
{
    if (desc == NULL)
        return true;
    if (!DVZ_STRUCT_VALID(desc, DvzGeometrySphereDesc, DVZ_GEOMETRY_SPHERE_DESC_KNOWN_FLAGS))
    {
        log_error("invalid DvzGeometrySphereDesc ABI prologue");
        return false;
    }
    return true;
}



static bool _geometry_surface_grid_desc_validate(const DvzGeometrySurfaceGridDesc* desc)
{
    if (desc == NULL)
        return false;
    if (!DVZ_STRUCT_VALID(
            desc, DvzGeometrySurfaceGridDesc, DVZ_GEOMETRY_SURFACE_GRID_DESC_KNOWN_FLAGS))
    {
        log_error("invalid DvzGeometrySurfaceGridDesc ABI prologue");
        return false;
    }
    return true;
}


static bool _geometry_disc_desc_validate(const DvzGeometryDiscDesc* desc)
{
    if (desc == NULL)
        return true;
    if (!DVZ_STRUCT_VALID(desc, DvzGeometryDiscDesc, DVZ_GEOMETRY_DISC_DESC_KNOWN_FLAGS))
    {
        log_error("invalid DvzGeometryDiscDesc ABI prologue");
        return false;
    }
    return true;
}



static bool _geometry_sector_desc_validate(const DvzGeometrySectorDesc* desc)
{
    if (desc == NULL)
        return true;
    if (!DVZ_STRUCT_VALID(desc, DvzGeometrySectorDesc, DVZ_GEOMETRY_SECTOR_DESC_KNOWN_FLAGS))
    {
        log_error("invalid DvzGeometrySectorDesc ABI prologue");
        return false;
    }
    return true;
}



static bool _geometry_regular_polygon_desc_validate(const DvzGeometryRegularPolygonDesc* desc)
{
    if (desc == NULL)
        return true;
    if (!DVZ_STRUCT_VALID(
            desc, DvzGeometryRegularPolygonDesc,
            DVZ_GEOMETRY_REGULAR_POLYGON_DESC_KNOWN_FLAGS))
    {
        log_error("invalid DvzGeometryRegularPolygonDesc ABI prologue");
        return false;
    }
    return true;
}



static bool _geometry_star_desc_validate(const DvzGeometryStarDesc* desc)
{
    if (desc == NULL)
        return true;
    if (!DVZ_STRUCT_VALID(desc, DvzGeometryStarDesc, DVZ_GEOMETRY_STAR_DESC_KNOWN_FLAGS))
    {
        log_error("invalid DvzGeometryStarDesc ABI prologue");
        return false;
    }
    return true;
}



static bool _geometry_cylinder_desc_validate(const DvzGeometryCylinderDesc* desc)
{
    if (desc == NULL)
        return true;
    if (!DVZ_STRUCT_VALID(desc, DvzGeometryCylinderDesc, DVZ_GEOMETRY_CYLINDER_DESC_KNOWN_FLAGS))
    {
        log_error("invalid DvzGeometryCylinderDesc ABI prologue");
        return false;
    }
    return true;
}



static bool _geometry_cone_desc_validate(const DvzGeometryConeDesc* desc)
{
    if (desc == NULL)
        return true;
    if (!DVZ_STRUCT_VALID(desc, DvzGeometryConeDesc, DVZ_GEOMETRY_CONE_DESC_KNOWN_FLAGS))
    {
        log_error("invalid DvzGeometryConeDesc ABI prologue");
        return false;
    }
    return true;
}



static bool _geometry_torus_desc_validate(const DvzGeometryTorusDesc* desc)
{
    if (desc == NULL)
        return true;
    if (!DVZ_STRUCT_VALID(desc, DvzGeometryTorusDesc, DVZ_GEOMETRY_TORUS_DESC_KNOWN_FLAGS))
    {
        log_error("invalid DvzGeometryTorusDesc ABI prologue");
        return false;
    }
    return true;
}



static bool _geometry_arrow_desc_validate(const DvzGeometryArrowDesc* desc)
{
    if (desc == NULL)
        return true;
    if (!DVZ_STRUCT_VALID(desc, DvzGeometryArrowDesc, DVZ_GEOMETRY_ARROW_DESC_KNOWN_FLAGS))
    {
        log_error("invalid DvzGeometryArrowDesc ABI prologue");
        return false;
    }
    return true;
}



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
static void _geom_color_or_default(const DvzColor color, DvzColor* out)
{
    ANN(out);
    if (color.r == 0 && color.g == 0 && color.b == 0 && color.a == 0)
    {
        *out = dvz_color_rgb(255, 255, 255);
        return;
    }

    *out = color;
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
    ASSERT(index < geometry->vertex_count);

    geometry->positions[index][0] = position[0];
    geometry->positions[index][1] = position[1];
    geometry->positions[index][2] = position[2];

    geometry->normals[index][0] = normal[0];
    geometry->normals[index][1] = normal[1];
    geometry->normals[index][2] = normal[2];

    geometry->texcoords[index][0] = texcoord[0];
    geometry->texcoords[index][1] = texcoord[1];

    geometry->colors[index] = color;
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
 * Fill triangle-fan indices where vertex zero is the center.
 *
 * @param geometry the geometry
 * @param triangle_count number of fan triangles
 */
static void _geom_set_fan_indices(DvzGeometry* geometry, uint32_t triangle_count)
{
    ANN(geometry);
    for (uint32_t i = 0; i < triangle_count; i++)
    {
        const uint32_t next = i + 2u <= triangle_count ? i + 2u : 1u;
        _geom_set_index(geometry, 3u * i + 0u, 0u);
        _geom_set_index(geometry, 3u * i + 1u, i + 1u);
        _geom_set_index(geometry, 3u * i + 2u, next);
    }
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
    for (uint32_t i = 0; i < geometry->vertex_count; i++)
    {
        geometry->colors[i] = color;
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
 * Linearly interpolate two F64 3D positions.
 *
 * @param a first position
 * @param b second position
 * @param t interpolation coordinate
 * @param out output position
 */
static void _geom_dvec3_lerp(const dvec3 a, const dvec3 b, double t, dvec3 out)
{
    ANN(a);
    ANN(b);
    ANN(out);
    out[0] = a[0] + t * (b[0] - a[0]);
    out[1] = a[1] + t * (b[1] - a[1]);
    out[2] = a[2] + t * (b[2] - a[2]);
}



/**
 * Return whether two F64 3D positions are equal enough for contour extraction.
 *
 * @param a first position
 * @param b second position
 * @return whether the positions are nearly equal
 */
static bool _geom_dvec3_nearly_equal(const dvec3 a, const dvec3 b)
{
    ANN(a);
    ANN(b);
    const dvec3 d = {a[0] - b[0], a[1] - b[1], a[2] - b[2]};
    return _geom_dvec3_norm2(d) <= EPSILON * EPSILON;
}



/**
 * Compute a flat triangle normal from three points.
 *
 * @param a first point
 * @param b second point
 * @param c third point
 * @param out output normal
 */
static void _geom_triangle_normal_from_points(const dvec3 a, const dvec3 b, const dvec3 c, dvec3 out)
{
    ANN(a);
    ANN(b);
    ANN(c);
    ANN(out);

    const dvec3 u = {b[0] - a[0], b[1] - a[1], b[2] - a[2]};
    const dvec3 v = {c[0] - a[0], c[1] - a[1], c[2] - a[2]};
    _geom_dvec3_cross(u, v, out);
    if (!_geom_dvec3_normalize(out))
    {
        out[0] = 0.0;
        out[1] = 0.0;
        out[2] = 1.0;
    }
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

    const double* p0 = geometry->positions[i0];
    const double* p1 = geometry->positions[i1];
    const double* p2 = geometry->positions[i2];
    dvec3 a = {p1[0] - p0[0], p1[1] - p0[1], p1[2] - p0[2]};
    dvec3 b = {p2[0] - p0[0], p2[1] - p0[1], p2[2] - p0[2]};
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
    const DvzGeometrySurfaceGridDesc* desc, DvzGeometrySurfaceGridDesc* out, DvzColor* color)
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
 * Return whether a geometry has valid triangle-list indices.
 *
 * @param geometry the geometry
 * @return whether indexed triangles are available
 */
static bool _geom_valid_indexed_triangles(const DvzGeometry* geometry)
{
    if (geometry == NULL || geometry->vertex_count == 0 || geometry->index_count == 0 ||
        geometry->indices == NULL || geometry->index_count % 3 != 0)
    {
        return false;
    }

    for (uint32_t i = 0; i < geometry->index_count; i++)
    {
        if (geometry->indices[i] >= geometry->vertex_count)
            return false;
    }
    return true;
}



/**
 * Compare derived edge candidates.
 *
 * @param ap first edge candidate
 * @param bp second edge candidate
 * @return sort comparison result
 */
static int _geom_edge_candidate_compare(const void* ap, const void* bp)
{
    const struct _GeomEdgeCandidate* a = (const struct _GeomEdgeCandidate*)ap;
    const struct _GeomEdgeCandidate* b = (const struct _GeomEdgeCandidate*)bp;
    if (a->v0 < b->v0)
        return -1;
    if (a->v0 > b->v0)
        return +1;
    if (a->v1 < b->v1)
        return -1;
    if (a->v1 > b->v1)
        return +1;
    if (a->face < b->face)
        return -1;
    if (a->face > b->face)
        return +1;
    return 0;
}



/**
 * Store one canonicalized triangle edge candidate.
 *
 * @param candidates edge candidate array
 * @param offset target offset
 * @param a first endpoint index
 * @param b second endpoint index
 * @param face source triangle index
 */
static void _geom_edge_candidate_set(
    struct _GeomEdgeCandidate* candidates, uint32_t offset, DvzIndex a, DvzIndex b, uint32_t face)
{
    ANN(candidates);
    candidates[offset].v0 = a < b ? a : b;
    candidates[offset].v1 = a < b ? b : a;
    candidates[offset].face = face;
}



/**
 * Append one contour intersection point for a triangle edge when present.
 *
 * @param geometry the geometry
 * @param ia first vertex index
 * @param ib second vertex index
 * @param sa first scalar value
 * @param sb second scalar value
 * @param level contour level
 * @param points output point array
 * @param count output point count
 */
static void _geom_contour_intersection(
    const DvzGeometry* geometry, DvzIndex ia, DvzIndex ib, double sa, double sb, double level,
    dvec3 points[3], uint32_t* count)
{
    ANN(geometry);
    ANN(points);
    ANN(count);
    ASSERT(ia < geometry->vertex_count);
    ASSERT(ib < geometry->vertex_count);
    if (*count >= 3 || sa == sb)
        return;

    const double min_value = sa < sb ? sa : sb;
    const double max_value = sa > sb ? sa : sb;
    if (level < min_value || level >= max_value)
        return;

    const double t = (level - sa) / (sb - sa);
    if (!isfinite(t) || t < -EPSILON || t > 1.0 + EPSILON)
        return;

    _geom_dvec3_lerp(geometry->positions[ia], geometry->positions[ib], CLIP(t, 0.0, 1.0),
                     points[*count]);
    *count += 1;
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
 * Return a default cube geometry descriptor.
 */
DvzGeometryCubeDesc dvz_geometry_cube_desc(void)
{
    return (DvzGeometryCubeDesc){
        DVZ_STRUCT_INIT_FIELDS(DvzGeometryCubeDesc),
        .size = 1.0,
    };
}



/**
 * Return a default plane geometry descriptor.
 */
DvzGeometryPlaneDesc dvz_geometry_plane_desc(void)
{
    return (DvzGeometryPlaneDesc){
        DVZ_STRUCT_INIT_FIELDS(DvzGeometryPlaneDesc),
        .width = 1.0,
        .height = 1.0,
    };
}



/**
 * Return a default sphere geometry descriptor.
 */
DvzGeometrySphereDesc dvz_geometry_sphere_desc(void)
{
    return (DvzGeometrySphereDesc){
        DVZ_STRUCT_INIT_FIELDS(DvzGeometrySphereDesc),
        .radius = 1.0,
        .rings = DVZ_GEOM_SPHERE_DEFAULT_RINGS,
        .sectors = DVZ_GEOM_SPHERE_DEFAULT_SECTORS,
    };
}



/**
 * Return a default surface-grid geometry descriptor.
 */
DvzGeometrySurfaceGridDesc dvz_geometry_surface_grid_desc(void)
{
    return (DvzGeometrySurfaceGridDesc){
        DVZ_STRUCT_INIT_FIELDS(DvzGeometrySurfaceGridDesc),
        .col_basis = {1.0, 0.0, 0.0},
        .row_basis = {0.0, 1.0, 0.0},
        .height_axis = {0.0, 0.0, 1.0},
        .height_scale = 1.0,
    };
}


/**
 * Return a default disc geometry descriptor.
 *
 * @return initialized disc descriptor
 */
DvzGeometryDiscDesc dvz_geometry_disc_desc(void)
{
    return (DvzGeometryDiscDesc){
        DVZ_STRUCT_INIT_FIELDS(DvzGeometryDiscDesc),
        .radius = 1.0,
        .segments = DVZ_GEOM_DISC_DEFAULT_SEGMENTS,
    };
}



/**
 * Return a default sector geometry descriptor.
 *
 * @return initialized sector descriptor
 */
DvzGeometrySectorDesc dvz_geometry_sector_desc(void)
{
    return (DvzGeometrySectorDesc){
        DVZ_STRUCT_INIT_FIELDS(DvzGeometrySectorDesc),
        .radius = 1.0,
        .sweep_angle = 0.5 * M_PI,
        .segments = DVZ_GEOM_SECTOR_DEFAULT_SEGMENTS,
    };
}



/**
 * Return a default regular-polygon geometry descriptor.
 *
 * @return initialized regular-polygon descriptor
 */
DvzGeometryRegularPolygonDesc dvz_geometry_regular_polygon_desc(void)
{
    return (DvzGeometryRegularPolygonDesc){
        DVZ_STRUCT_INIT_FIELDS(DvzGeometryRegularPolygonDesc),
        .radius = 1.0,
        .sides = DVZ_GEOM_POLYGON_DEFAULT_SIDES,
    };
}



/**
 * Return a default star geometry descriptor.
 *
 * @return initialized star descriptor
 */
DvzGeometryStarDesc dvz_geometry_star_desc(void)
{
    return (DvzGeometryStarDesc){
        DVZ_STRUCT_INIT_FIELDS(DvzGeometryStarDesc),
        .outer_radius = 1.0,
        .inner_radius = 0.45,
        .points = DVZ_GEOM_STAR_DEFAULT_POINTS,
    };
}



/**
 * Return a default cylinder geometry descriptor.
 *
 * @return initialized cylinder descriptor
 */
DvzGeometryCylinderDesc dvz_geometry_cylinder_desc(void)
{
    return (DvzGeometryCylinderDesc){
        DVZ_STRUCT_INIT_FIELDS(DvzGeometryCylinderDesc),
        .radius = 0.5,
        .height = 1.0,
        .sectors = DVZ_GEOM_REVOLUTION_DEFAULT_SECTORS,
    };
}



/**
 * Return a default cone geometry descriptor.
 *
 * @return initialized cone descriptor
 */
DvzGeometryConeDesc dvz_geometry_cone_desc(void)
{
    return (DvzGeometryConeDesc){
        DVZ_STRUCT_INIT_FIELDS(DvzGeometryConeDesc),
        .radius = 0.5,
        .height = 1.0,
        .sectors = DVZ_GEOM_REVOLUTION_DEFAULT_SECTORS,
    };
}



/**
 * Return a default torus geometry descriptor.
 *
 * @return initialized torus descriptor
 */
DvzGeometryTorusDesc dvz_geometry_torus_desc(void)
{
    return (DvzGeometryTorusDesc){
        DVZ_STRUCT_INIT_FIELDS(DvzGeometryTorusDesc),
        .major_radius = 0.65,
        .minor_radius = 0.18,
        .rings = DVZ_GEOM_TORUS_DEFAULT_RINGS,
        .sectors = DVZ_GEOM_TORUS_DEFAULT_SECTORS,
    };
}



/**
 * Return a default arrow geometry descriptor.
 *
 * @return initialized arrow descriptor
 */
DvzGeometryArrowDesc dvz_geometry_arrow_desc(void)
{
    return (DvzGeometryArrowDesc){
        DVZ_STRUCT_INIT_FIELDS(DvzGeometryArrowDesc),
        .length = 1.25,
        .shaft_radius = 0.08,
        .head_radius = 0.20,
        .head_length = 0.36,
        .sectors = DVZ_GEOM_REVOLUTION_DEFAULT_SECTORS,
    };
}



#define DVZ_FFI_DESC_WRAPPER(name, type)                                                        \
    bool dvz_ffi_##name(type* out)                                                              \
    {                                                                                            \
        if (out == NULL)                                                                         \
            return false;                                                                        \
        *out = dvz_##name();                                                                     \
        return true;                                                                             \
    }

DVZ_FFI_DESC_WRAPPER(geometry_arrow_desc, DvzGeometryArrowDesc)
DVZ_FFI_DESC_WRAPPER(geometry_cone_desc, DvzGeometryConeDesc)
DVZ_FFI_DESC_WRAPPER(geometry_cube_desc, DvzGeometryCubeDesc)
DVZ_FFI_DESC_WRAPPER(geometry_cylinder_desc, DvzGeometryCylinderDesc)
DVZ_FFI_DESC_WRAPPER(geometry_disc_desc, DvzGeometryDiscDesc)
DVZ_FFI_DESC_WRAPPER(geometry_plane_desc, DvzGeometryPlaneDesc)
DVZ_FFI_DESC_WRAPPER(geometry_regular_polygon_desc, DvzGeometryRegularPolygonDesc)
DVZ_FFI_DESC_WRAPPER(geometry_sector_desc, DvzGeometrySectorDesc)
DVZ_FFI_DESC_WRAPPER(geometry_sphere_desc, DvzGeometrySphereDesc)
DVZ_FFI_DESC_WRAPPER(geometry_star_desc, DvzGeometryStarDesc)
DVZ_FFI_DESC_WRAPPER(geometry_surface_grid_desc, DvzGeometrySurfaceGridDesc)
DVZ_FFI_DESC_WRAPPER(geometry_torus_desc, DvzGeometryTorusDesc)

#undef DVZ_FFI_DESC_WRAPPER



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
        const double* p = geometry->positions[i];
        xmin = p[0] < xmin ? p[0] : xmin;
        xmax = p[0] > xmax ? p[0] : xmax;
        ymin = p[1] < ymin ? p[1] : ymin;
        ymax = p[1] > ymax ? p[1] : ymax;
        zmin = p[2] < zmin ? p[2] : zmin;
        zmax = p[2] > zmax ? p[2] : zmax;
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
DvzResult dvz_geometry_compute_normals(DvzGeometry* geometry)
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
DvzResult dvz_geometry_transform(DvzGeometry* geometry, dmat4 transform)
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
 * Derive a unique edge list from indexed triangle geometry.
 *
 * @param geometry the geometry
 * @return the derived edge list, or NULL on invalid input or allocation failure
 */
DvzGeometryEdges* dvz_geometry_edges(const DvzGeometry* geometry)
{
    if (!_geom_valid_indexed_triangles(geometry))
        return NULL;

    const uint32_t triangle_count = geometry->index_count / 3;
    uint64_t candidate_count_u64 = 0;
    if (_dvz_mul_u64_overflows((uint64_t)triangle_count, 3, &candidate_count_u64) ||
        candidate_count_u64 > UINT32_MAX ||
        !_geom_allocation_valid((uint32_t)candidate_count_u64, sizeof(struct _GeomEdgeCandidate)))
    {
        return NULL;
    }
    const uint32_t candidate_count = (uint32_t)candidate_count_u64;

    struct _GeomEdgeCandidate* candidates =
        (struct _GeomEdgeCandidate*)dvz_calloc(candidate_count, sizeof(struct _GeomEdgeCandidate));
    if (candidates == NULL)
        return NULL;

    for (uint32_t face = 0; face < triangle_count; face++)
    {
        const uint32_t offset = 3 * face;
        const DvzIndex i0 = geometry->indices[offset + 0];
        const DvzIndex i1 = geometry->indices[offset + 1];
        const DvzIndex i2 = geometry->indices[offset + 2];
        _geom_edge_candidate_set(candidates, offset + 0, i0, i1, face);
        _geom_edge_candidate_set(candidates, offset + 1, i1, i2, face);
        _geom_edge_candidate_set(candidates, offset + 2, i2, i0, face);
    }

    qsort(candidates, candidate_count, sizeof(struct _GeomEdgeCandidate),
          _geom_edge_candidate_compare);

    uint32_t edge_count = 0;
    for (uint32_t i = 0; i < candidate_count;)
    {
        const DvzIndex v0 = candidates[i].v0;
        const DvzIndex v1 = candidates[i].v1;
        do
            i++;
        while (i < candidate_count && candidates[i].v0 == v0 && candidates[i].v1 == v1);
        edge_count++;
    }

    DvzGeometryEdges* out = (DvzGeometryEdges*)dvz_calloc(1, sizeof(DvzGeometryEdges));
    if (out == NULL)
    {
        dvz_free(candidates);
        return NULL;
    }

    out->edges = (DvzGeometryEdge*)dvz_calloc(edge_count, sizeof(DvzGeometryEdge));
    if (out->edges == NULL)
    {
        dvz_free(candidates);
        dvz_free(out);
        return NULL;
    }
    out->edge_count = edge_count;

    uint32_t edge_index = 0;
    for (uint32_t i = 0; i < candidate_count;)
    {
        DvzGeometryEdge* edge = &out->edges[edge_index++];
        edge->v0 = candidates[i].v0;
        edge->v1 = candidates[i].v1;
        edge->face0 = UINT32_MAX;
        edge->face1 = UINT32_MAX;

        while (i < candidate_count && candidates[i].v0 == edge->v0 && candidates[i].v1 == edge->v1)
        {
            if (edge->adjacent_count == 0)
                edge->face0 = candidates[i].face;
            else if (edge->adjacent_count == 1)
                edge->face1 = candidates[i].face;
            edge->adjacent_count++;
            i++;
        }

        if (edge->adjacent_count == 1)
            edge->flags |= DVZ_GEOMETRY_EDGE_BOUNDARY;
        else if (edge->adjacent_count > 2)
            edge->flags |= DVZ_GEOMETRY_EDGE_NONMANIFOLD;
    }

    dvz_free(candidates);
    return out;
}



/**
 * Destroy a derived geometry edge list.
 *
 * @param edges the edge list
 */
void dvz_geometry_edges_destroy(DvzGeometryEdges* edges)
{
    if (edges == NULL)
        return;

    dvz_free(edges->edges);
    dvz_memset(edges, sizeof(DvzGeometryEdges), 0, sizeof(DvzGeometryEdges));
    dvz_free(edges);
}



/**
 * Extract contour line segments from indexed triangle geometry and per-vertex scalar values.
 *
 * @param geometry the geometry
 * @param values scalar value per vertex
 * @param value_count number of scalar values
 * @param levels contour levels
 * @param level_count number of contour levels
 * @return the extracted contour segments, or NULL on invalid input or allocation failure
 */
DvzGeometryContours* dvz_geometry_contours(
    const DvzGeometry* geometry, const double* values, uint32_t value_count, const double* levels,
    uint32_t level_count)
{
    if (!_geom_valid_indexed_triangles(geometry) || geometry->positions == NULL || values == NULL ||
        levels == NULL || value_count != geometry->vertex_count || level_count == 0)
    {
        return NULL;
    }

    const uint32_t triangle_count = geometry->index_count / 3;
    uint64_t max_segments_u64 = 0;
    if (_dvz_mul_u64_overflows((uint64_t)triangle_count, (uint64_t)level_count, &max_segments_u64) ||
        max_segments_u64 > UINT32_MAX ||
        !_geom_allocation_valid(
            (uint32_t)max_segments_u64, sizeof(DvzGeometryContourSegment)))
    {
        return NULL;
    }
    const uint32_t max_segments = (uint32_t)max_segments_u64;

    DvzGeometryContours* out = (DvzGeometryContours*)dvz_calloc(1, sizeof(DvzGeometryContours));
    if (out == NULL)
        return NULL;

    if (max_segments > 0)
    {
        out->segments =
            (DvzGeometryContourSegment*)dvz_calloc(max_segments, sizeof(DvzGeometryContourSegment));
        if (out->segments == NULL)
        {
            dvz_free(out);
            return NULL;
        }
    }

    for (uint32_t face = 0; face < triangle_count; face++)
    {
        const uint32_t offset = 3 * face;
        const DvzIndex i0 = geometry->indices[offset + 0];
        const DvzIndex i1 = geometry->indices[offset + 1];
        const DvzIndex i2 = geometry->indices[offset + 2];
        const double s0 = values[i0];
        const double s1 = values[i1];
        const double s2 = values[i2];
        if (!isfinite(s0) || !isfinite(s1) || !isfinite(s2))
            continue;

        for (uint32_t level_index = 0; level_index < level_count; level_index++)
        {
            const double level = levels[level_index];
            if (!isfinite(level))
                continue;

            dvec3 points[3] = {{0}};
            uint32_t point_count = 0;
            _geom_contour_intersection(
                geometry, i0, i1, s0, s1, level, points, &point_count);
            _geom_contour_intersection(
                geometry, i1, i2, s1, s2, level, points, &point_count);
            _geom_contour_intersection(
                geometry, i2, i0, s2, s0, level, points, &point_count);

            if (point_count != 2 || _geom_dvec3_nearly_equal(points[0], points[1]))
                continue;

            ASSERT(out->segment_count < max_segments);
            DvzGeometryContourSegment* segment = &out->segments[out->segment_count++];
            dvz_memcpy(segment->p0, sizeof(dvec3), points[0], sizeof(dvec3));
            dvz_memcpy(segment->p1, sizeof(dvec3), points[1], sizeof(dvec3));
            segment->level = level;
            segment->level_index = level_index;
            segment->face_index = face;
        }
    }

    return out;
}



/**
 * Destroy extracted contour segments.
 *
 * @param contours the contour segment list
 */
void dvz_geometry_contours_destroy(DvzGeometryContours* contours)
{
    if (contours == NULL)
        return;

    dvz_free(contours->segments);
    dvz_memset(contours, sizeof(DvzGeometryContours), 0, sizeof(DvzGeometryContours));
    dvz_free(contours);
}



/**
 * Create an indexed cube geometry.
 *
 * @param desc optional cube descriptor
 * @return the new geometry, or NULL on failure
 */
DvzGeometry* dvz_geom_cube(const DvzGeometryCubeDesc* desc)
{
    if (!_geometry_cube_desc_validate(desc))
        return NULL;
    DvzGeometryCubeDesc cfg = dvz_geometry_cube_desc();
    if (desc != NULL)
        cfg = *desc;
    if (cfg.size <= 0)
        return NULL;
    if (cfg.face_colors != NULL && cfg.face_color_count < DVZ_GEOM_CUBE_FACE_COUNT)
        return NULL;

    DvzColor color = {0};
    _geom_color_or_default(cfg.color, &color);

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

    const dvec3 faces[DVZ_GEOM_CUBE_FACE_COUNT][4] = {
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
    const dvec3 normals[DVZ_GEOM_CUBE_FACE_COUNT] = {
        {+1, 0, 0}, {-1, 0, 0}, {0, +1, 0}, {0, -1, 0}, {0, 0, +1}, {0, 0, -1}};

    for (uint32_t face = 0; face < DVZ_GEOM_CUBE_FACE_COUNT; face++)
    {
        DvzColor face_color = {0};
        const DvzColor* vertex_color = &color;
        if (cfg.face_colors != NULL)
        {
            _geom_color_or_default(cfg.face_colors[face], &face_color);
            vertex_color = &face_color;
        }

        const uint32_t base = 4 * face;
        for (uint32_t j = 0; j < 4; j++)
            _geom_set_vertex(
                geometry, base + j, faces[face][j], normals[face], uv[j], *vertex_color);

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
    if (!_geometry_plane_desc_validate(desc))
        return NULL;
    DvzGeometryPlaneDesc cfg = dvz_geometry_plane_desc();
    if (desc != NULL)
        cfg = *desc;
    if (cfg.width <= 0 || cfg.height <= 0)
        return NULL;

    DvzColor color = {0};
    _geom_color_or_default(cfg.color, &color);

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
    if (!_geometry_sphere_desc_validate(desc))
        return NULL;
    DvzGeometrySphereDesc cfg = dvz_geometry_sphere_desc();
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
    _geom_color_or_default(cfg.color, &color);

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
    if (!_geometry_surface_grid_desc_validate(desc))
        return NULL;

    DvzGeometrySurfaceGridDesc cfg = dvz_geometry_surface_grid_desc();
    DvzColor fallback_color = {0};
    _geom_surface_grid_config(desc, &cfg, &fallback_color);

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
DvzResult dvz_geom_surface_grid_update_heights(
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



/**
 * Create an indexed XY disc geometry.
 *
 * @param desc optional disc descriptor
 * @return the new geometry, or NULL on failure
 */
DvzGeometry* dvz_geom_disc(const DvzGeometryDiscDesc* desc)
{
    if (!_geometry_disc_desc_validate(desc))
        return NULL;
    DvzGeometryDiscDesc cfg = dvz_geometry_disc_desc();
    if (desc != NULL)
        cfg = *desc;
    if (cfg.radius <= 0.0)
        return NULL;
    if (cfg.segments == 0)
        cfg.segments = DVZ_GEOM_DISC_DEFAULT_SEGMENTS;
    if (cfg.segments < 3u)
        return NULL;

    DvzColor color = {0};
    _geom_color_or_default(cfg.color, &color);

    DvzGeometry* geometry = dvz_geometry(cfg.segments + 1u, 3u * cfg.segments);
    if (geometry == NULL)
        return NULL;
    geometry->type = DVZ_GEOMETRY_PLANE;
    geometry->flags = DVZ_GEOMETRY_INDEXING_TRIANGLES;

    const dvec3 normal = {0.0, 0.0, 1.0};
    _geom_set_vertex(geometry, 0, cfg.center, normal, (dvec2){0.5, 0.5}, color);
    for (uint32_t i = 0; i < cfg.segments; i++)
    {
        const double t = M_2PI * (double)i / (double)cfg.segments;
        const double c = cos(t);
        const double s = sin(t);
        const dvec3 position = {
            cfg.center[0] + cfg.radius * c,
            cfg.center[1] + cfg.radius * s,
            cfg.center[2],
        };
        const dvec2 uv = {0.5 + 0.5 * c, 0.5 + 0.5 * s};
        _geom_set_vertex(geometry, i + 1u, position, normal, uv, color);
    }
    _geom_set_fan_indices(geometry, cfg.segments);
    return geometry;
}



/**
 * Create an indexed XY sector geometry.
 *
 * @param desc optional sector descriptor
 * @return the new geometry, or NULL on failure
 */
DvzGeometry* dvz_geom_sector(const DvzGeometrySectorDesc* desc)
{
    if (!_geometry_sector_desc_validate(desc))
        return NULL;
    DvzGeometrySectorDesc cfg = dvz_geometry_sector_desc();
    if (desc != NULL)
        cfg = *desc;
    if (cfg.radius <= 0.0 || !isfinite(cfg.start_angle) || !isfinite(cfg.sweep_angle))
        return NULL;
    if (cfg.segments == 0)
        cfg.segments = DVZ_GEOM_SECTOR_DEFAULT_SEGMENTS;
    if (cfg.segments < 1u || fabs(cfg.sweep_angle) <= EPSILON)
        return NULL;

    DvzColor color = {0};
    _geom_color_or_default(cfg.color, &color);

    DvzGeometry* geometry = dvz_geometry(cfg.segments + 2u, 3u * cfg.segments);
    if (geometry == NULL)
        return NULL;
    geometry->type = DVZ_GEOMETRY_PLANE;
    geometry->flags = DVZ_GEOMETRY_INDEXING_TRIANGLES;

    const dvec3 normal = {0.0, 0.0, cfg.sweep_angle >= 0.0 ? 1.0 : -1.0};
    _geom_set_vertex(geometry, 0, cfg.center, normal, (dvec2){0.5, 0.5}, color);
    for (uint32_t i = 0; i <= cfg.segments; i++)
    {
        const double u = (double)i / (double)cfg.segments;
        const double t = cfg.start_angle + cfg.sweep_angle * u;
        const double c = cos(t);
        const double s = sin(t);
        const dvec3 position = {
            cfg.center[0] + cfg.radius * c,
            cfg.center[1] + cfg.radius * s,
            cfg.center[2],
        };
        const dvec2 uv = {0.5 + 0.5 * c, 0.5 + 0.5 * s};
        _geom_set_vertex(geometry, i + 1u, position, normal, uv, color);
    }

    for (uint32_t i = 0; i < cfg.segments; i++)
    {
        _geom_set_index(geometry, 3u * i + 0u, 0u);
        if (cfg.sweep_angle >= 0.0)
        {
            _geom_set_index(geometry, 3u * i + 1u, i + 1u);
            _geom_set_index(geometry, 3u * i + 2u, i + 2u);
        }
        else
        {
            _geom_set_index(geometry, 3u * i + 1u, i + 2u);
            _geom_set_index(geometry, 3u * i + 2u, i + 1u);
        }
    }
    return geometry;
}



/**
 * Create an indexed XY regular-polygon geometry.
 *
 * @param desc optional regular-polygon descriptor
 * @return the new geometry, or NULL on failure
 */
DvzGeometry* dvz_geom_regular_polygon(const DvzGeometryRegularPolygonDesc* desc)
{
    if (!_geometry_regular_polygon_desc_validate(desc))
        return NULL;
    DvzGeometryRegularPolygonDesc cfg = dvz_geometry_regular_polygon_desc();
    if (desc != NULL)
        cfg = *desc;
    if (cfg.sides == 0)
        cfg.sides = DVZ_GEOM_POLYGON_DEFAULT_SIDES;
    if (cfg.radius <= 0.0 || cfg.sides < 3u)
        return NULL;

    return dvz_geom_disc(&(DvzGeometryDiscDesc){
        DVZ_STRUCT_INIT_FIELDS(DvzGeometryDiscDesc),
        .center = {cfg.center[0], cfg.center[1], cfg.center[2]},
        .radius = cfg.radius,
        .segments = cfg.sides,
        .color = cfg.color,
    });
}



/**
 * Create an indexed XY star geometry.
 *
 * @param desc optional star descriptor
 * @return the new geometry, or NULL on failure
 */
DvzGeometry* dvz_geom_star(const DvzGeometryStarDesc* desc)
{
    if (!_geometry_star_desc_validate(desc))
        return NULL;
    DvzGeometryStarDesc cfg = dvz_geometry_star_desc();
    if (desc != NULL)
        cfg = *desc;
    if (cfg.points == 0)
        cfg.points = DVZ_GEOM_STAR_DEFAULT_POINTS;
    if (cfg.outer_radius <= 0.0 || cfg.inner_radius <= 0.0 || cfg.inner_radius >= cfg.outer_radius ||
        cfg.points < 3u || cfg.points > UINT32_MAX / 2u)
    {
        return NULL;
    }

    DvzColor color = {0};
    _geom_color_or_default(cfg.color, &color);

    const uint32_t outer_count = 2u * cfg.points;
    DvzGeometry* geometry = dvz_geometry(outer_count + 1u, 3u * outer_count);
    if (geometry == NULL)
        return NULL;
    geometry->type = DVZ_GEOMETRY_PLANE;
    geometry->flags = DVZ_GEOMETRY_INDEXING_TRIANGLES;

    const dvec3 normal = {0.0, 0.0, 1.0};
    _geom_set_vertex(geometry, 0, cfg.center, normal, (dvec2){0.5, 0.5}, color);
    for (uint32_t i = 0; i < outer_count; i++)
    {
        const double radius = i % 2u == 0 ? cfg.outer_radius : cfg.inner_radius;
        const double t = -0.5 * M_PI + M_2PI * (double)i / (double)outer_count;
        const double c = cos(t);
        const double s = sin(t);
        const dvec3 position = {
            cfg.center[0] + radius * c,
            cfg.center[1] + radius * s,
            cfg.center[2],
        };
        const dvec2 uv = {0.5 + 0.5 * c, 0.5 + 0.5 * s};
        _geom_set_vertex(geometry, i + 1u, position, normal, uv, color);
    }
    _geom_set_fan_indices(geometry, outer_count);
    return geometry;
}



/**
 * Create an indexed Z-axis cylinder geometry.
 *
 * @param desc optional cylinder descriptor
 * @return the new geometry, or NULL on failure
 */
DvzGeometry* dvz_geom_cylinder(const DvzGeometryCylinderDesc* desc)
{
    if (!_geometry_cylinder_desc_validate(desc))
        return NULL;
    DvzGeometryCylinderDesc cfg = dvz_geometry_cylinder_desc();
    if (desc != NULL)
        cfg = *desc;
    if (cfg.sectors == 0)
        cfg.sectors = DVZ_GEOM_REVOLUTION_DEFAULT_SECTORS;
    if (cfg.radius <= 0.0 || cfg.height <= 0.0 || cfg.sectors < 3u)
        return NULL;

    DvzColor color = {0};
    _geom_color_or_default(cfg.color, &color);

    const uint32_t side_vertices = 2u * (cfg.sectors + 1u);
    const uint32_t cap_vertices = cfg.sectors + 2u;
    const uint32_t vertex_count = side_vertices + 2u * cap_vertices;
    const uint32_t index_count = 12u * cfg.sectors;
    DvzGeometry* geometry = dvz_geometry(vertex_count, index_count);
    if (geometry == NULL)
        return NULL;
    geometry->type = DVZ_GEOMETRY_CYLINDER;
    geometry->flags = DVZ_GEOMETRY_INDEXING_TRIANGLES;

    const double z0 = cfg.center[2] - 0.5 * cfg.height;
    const double z1 = cfg.center[2] + 0.5 * cfg.height;
    for (uint32_t i = 0; i <= cfg.sectors; i++)
    {
        const double u = (double)i / (double)cfg.sectors;
        const double t = M_2PI * u;
        const double c = cos(t);
        const double s = sin(t);
        const dvec3 normal = {c, s, 0.0};
        _geom_set_vertex(
            geometry, 2u * i + 0u,
            (dvec3){cfg.center[0] + cfg.radius * c, cfg.center[1] + cfg.radius * s, z0},
            normal, (dvec2){u, 0.0}, color);
        _geom_set_vertex(
            geometry, 2u * i + 1u,
            (dvec3){cfg.center[0] + cfg.radius * c, cfg.center[1] + cfg.radius * s, z1},
            normal, (dvec2){u, 1.0}, color);
    }

    uint32_t index = 0;
    for (uint32_t i = 0; i < cfg.sectors; i++)
    {
        const uint32_t b0 = 2u * i;
        const uint32_t t0 = b0 + 1u;
        const uint32_t b1 = b0 + 2u;
        const uint32_t t1 = b0 + 3u;
        _geom_set_index(geometry, index++, b0);
        _geom_set_index(geometry, index++, b1);
        _geom_set_index(geometry, index++, t1);
        _geom_set_index(geometry, index++, b0);
        _geom_set_index(geometry, index++, t1);
        _geom_set_index(geometry, index++, t0);
    }

    const uint32_t top_base = side_vertices;
    const uint32_t bottom_base = side_vertices + cap_vertices;
    _geom_set_vertex(
        geometry, top_base, (dvec3){cfg.center[0], cfg.center[1], z1}, (dvec3){0, 0, +1},
        (dvec2){0.5, 0.5}, color);
    _geom_set_vertex(
        geometry, bottom_base, (dvec3){cfg.center[0], cfg.center[1], z0}, (dvec3){0, 0, -1},
        (dvec2){0.5, 0.5}, color);
    for (uint32_t i = 0; i <= cfg.sectors; i++)
    {
        const double u = (double)i / (double)cfg.sectors;
        const double t = M_2PI * u;
        const double c = cos(t);
        const double s = sin(t);
        const dvec2 uv = {0.5 + 0.5 * c, 0.5 + 0.5 * s};
        _geom_set_vertex(
            geometry, top_base + 1u + i,
            (dvec3){cfg.center[0] + cfg.radius * c, cfg.center[1] + cfg.radius * s, z1},
            (dvec3){0, 0, +1}, uv, color);
        _geom_set_vertex(
            geometry, bottom_base + 1u + i,
            (dvec3){cfg.center[0] + cfg.radius * c, cfg.center[1] + cfg.radius * s, z0},
            (dvec3){0, 0, -1}, uv, color);
    }

    for (uint32_t i = 0; i < cfg.sectors; i++)
    {
        _geom_set_index(geometry, index++, top_base);
        _geom_set_index(geometry, index++, top_base + 1u + i);
        _geom_set_index(geometry, index++, top_base + 2u + i);
        _geom_set_index(geometry, index++, bottom_base);
        _geom_set_index(geometry, index++, bottom_base + 2u + i);
        _geom_set_index(geometry, index++, bottom_base + 1u + i);
    }
    return geometry;
}



/**
 * Create an indexed Z-axis cone geometry.
 *
 * @param desc optional cone descriptor
 * @return the new geometry, or NULL on failure
 */
DvzGeometry* dvz_geom_cone(const DvzGeometryConeDesc* desc)
{
    if (!_geometry_cone_desc_validate(desc))
        return NULL;
    DvzGeometryConeDesc cfg = dvz_geometry_cone_desc();
    if (desc != NULL)
        cfg = *desc;
    if (cfg.sectors == 0)
        cfg.sectors = DVZ_GEOM_REVOLUTION_DEFAULT_SECTORS;
    if (cfg.radius <= 0.0 || cfg.height <= 0.0 || cfg.sectors < 3u)
        return NULL;

    DvzColor color = {0};
    _geom_color_or_default(cfg.color, &color);

    const uint32_t side_vertices = 3u * cfg.sectors;
    const uint32_t cap_vertices = cfg.sectors + 2u;
    DvzGeometry* geometry = dvz_geometry(side_vertices + cap_vertices, 6u * cfg.sectors);
    if (geometry == NULL)
        return NULL;
    geometry->type = DVZ_GEOMETRY_CONE;
    geometry->flags = DVZ_GEOMETRY_INDEXING_TRIANGLES;

    const double z0 = cfg.center[2] - 0.5 * cfg.height;
    const double z1 = cfg.center[2] + 0.5 * cfg.height;
    const dvec3 apex = {cfg.center[0], cfg.center[1], z1};
    uint32_t vertex = 0;
    uint32_t index = 0;
    for (uint32_t i = 0; i < cfg.sectors; i++)
    {
        const double t0 = M_2PI * (double)i / (double)cfg.sectors;
        const double t1 = M_2PI * (double)(i + 1u) / (double)cfg.sectors;
        const dvec3 p0 = {cfg.center[0] + cfg.radius * cos(t0), cfg.center[1] + cfg.radius * sin(t0), z0};
        const dvec3 p1 = {cfg.center[0] + cfg.radius * cos(t1), cfg.center[1] + cfg.radius * sin(t1), z0};
        dvec3 normal = {0};
        _geom_triangle_normal_from_points(p0, p1, apex, normal);
        _geom_set_vertex(geometry, vertex + 0u, p0, normal, (dvec2){0.0, 0.0}, color);
        _geom_set_vertex(geometry, vertex + 1u, p1, normal, (dvec2){1.0, 0.0}, color);
        _geom_set_vertex(geometry, vertex + 2u, apex, normal, (dvec2){0.5, 1.0}, color);
        _geom_set_index(geometry, index++, vertex + 0u);
        _geom_set_index(geometry, index++, vertex + 1u);
        _geom_set_index(geometry, index++, vertex + 2u);
        vertex += 3u;
    }

    const uint32_t cap_base = side_vertices;
    _geom_set_vertex(
        geometry, cap_base, (dvec3){cfg.center[0], cfg.center[1], z0}, (dvec3){0, 0, -1},
        (dvec2){0.5, 0.5}, color);
    for (uint32_t i = 0; i <= cfg.sectors; i++)
    {
        const double u = (double)i / (double)cfg.sectors;
        const double t = M_2PI * u;
        const double c = cos(t);
        const double s = sin(t);
        _geom_set_vertex(
            geometry, cap_base + 1u + i,
            (dvec3){cfg.center[0] + cfg.radius * c, cfg.center[1] + cfg.radius * s, z0},
            (dvec3){0, 0, -1}, (dvec2){0.5 + 0.5 * c, 0.5 + 0.5 * s}, color);
    }
    for (uint32_t i = 0; i < cfg.sectors; i++)
    {
        _geom_set_index(geometry, index++, cap_base);
        _geom_set_index(geometry, index++, cap_base + 2u + i);
        _geom_set_index(geometry, index++, cap_base + 1u + i);
    }
    return geometry;
}



/**
 * Create an indexed torus geometry around the Z axis.
 *
 * @param desc optional torus descriptor
 * @return the new geometry, or NULL on failure
 */
DvzGeometry* dvz_geom_torus(const DvzGeometryTorusDesc* desc)
{
    if (!_geometry_torus_desc_validate(desc))
        return NULL;
    DvzGeometryTorusDesc cfg = dvz_geometry_torus_desc();
    if (desc != NULL)
        cfg = *desc;
    if (cfg.rings == 0)
        cfg.rings = DVZ_GEOM_TORUS_DEFAULT_RINGS;
    if (cfg.sectors == 0)
        cfg.sectors = DVZ_GEOM_TORUS_DEFAULT_SECTORS;
    if (cfg.major_radius <= 0.0 || cfg.minor_radius <= 0.0 || cfg.rings < 3u || cfg.sectors < 3u)
        return NULL;

    const uint32_t cols = cfg.sectors + 1u;
    DvzGeometry* geometry = dvz_geometry((cfg.rings + 1u) * cols, 6u * cfg.rings * cfg.sectors);
    if (geometry == NULL)
        return NULL;
    geometry->type = DVZ_GEOMETRY_TORUS;
    geometry->flags = DVZ_GEOMETRY_INDEXING_TRIANGLES;

    DvzColor color = {0};
    _geom_color_or_default(cfg.color, &color);

    for (uint32_t ring = 0; ring <= cfg.rings; ring++)
    {
        const double u = (double)ring / (double)cfg.rings;
        const double phi = M_2PI * u;
        const double cp = cos(phi);
        const double sp = sin(phi);
        for (uint32_t sector = 0; sector <= cfg.sectors; sector++)
        {
            const double v = (double)sector / (double)cfg.sectors;
            const double theta = M_2PI * v;
            const double ct = cos(theta);
            const double st = sin(theta);
            const dvec3 normal = {cp * ct, sp * ct, st};
            const double radius = cfg.major_radius + cfg.minor_radius * ct;
            const dvec3 position = {
                cfg.center[0] + radius * cp,
                cfg.center[1] + radius * sp,
                cfg.center[2] + cfg.minor_radius * st,
            };
            _geom_set_vertex(geometry, ring * cols + sector, position, normal, (dvec2){u, v}, color);
        }
    }

    uint32_t index = 0;
    for (uint32_t ring = 0; ring < cfg.rings; ring++)
    {
        for (uint32_t sector = 0; sector < cfg.sectors; sector++)
        {
            const uint32_t i0 = ring * cols + sector;
            const uint32_t i1 = i0 + 1u;
            const uint32_t i2 = i0 + cols;
            const uint32_t i3 = i2 + 1u;
            _geom_set_index(geometry, index++, i0);
            _geom_set_index(geometry, index++, i2);
            _geom_set_index(geometry, index++, i3);
            _geom_set_index(geometry, index++, i0);
            _geom_set_index(geometry, index++, i3);
            _geom_set_index(geometry, index++, i1);
        }
    }
    return geometry;
}



/**
 * Create an indexed Z-axis arrow geometry.
 *
 * @param desc optional arrow descriptor
 * @return the new geometry, or NULL on failure
 */
DvzGeometry* dvz_geom_arrow(const DvzGeometryArrowDesc* desc)
{
    if (!_geometry_arrow_desc_validate(desc))
        return NULL;
    DvzGeometryArrowDesc cfg = dvz_geometry_arrow_desc();
    if (desc != NULL)
        cfg = *desc;
    if (cfg.sectors == 0)
        cfg.sectors = DVZ_GEOM_REVOLUTION_DEFAULT_SECTORS;
    if (
        cfg.length <= 0.0 || cfg.shaft_radius <= 0.0 || cfg.head_radius <= 0.0 ||
        cfg.head_length <= 0.0 || cfg.head_length >= cfg.length || cfg.sectors < 3u)
    {
        return NULL;
    }

    const double shaft_height = cfg.length - cfg.head_length;
    DvzGeometry* shaft = dvz_geom_cylinder(&(DvzGeometryCylinderDesc){
        DVZ_STRUCT_INIT_FIELDS(DvzGeometryCylinderDesc),
        .center = {cfg.center[0], cfg.center[1], cfg.center[2] - 0.5 * cfg.head_length},
        .radius = cfg.shaft_radius,
        .height = shaft_height,
        .sectors = cfg.sectors,
        .color = cfg.color,
    });
    DvzGeometry* head = dvz_geom_cone(&(DvzGeometryConeDesc){
        DVZ_STRUCT_INIT_FIELDS(DvzGeometryConeDesc),
        .center = {cfg.center[0], cfg.center[1], cfg.center[2] + 0.5 * shaft_height},
        .radius = cfg.head_radius,
        .height = cfg.head_length,
        .sectors = cfg.sectors,
        .color = cfg.color,
    });
    if (shaft == NULL || head == NULL)
    {
        if (shaft != NULL)
            dvz_geometry_destroy(shaft);
        if (head != NULL)
            dvz_geometry_destroy(head);
        return NULL;
    }

    const DvzGeometry* parts[] = {shaft, head};
    DvzGeometry* geometry = dvz_geometry_merge(2, parts);
    dvz_geometry_destroy(shaft);
    dvz_geometry_destroy(head);
    if (geometry != NULL)
        geometry->type = DVZ_GEOMETRY_ARROW;
    return geometry;
}
