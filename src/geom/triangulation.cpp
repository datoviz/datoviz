/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Polygon triangulation                                                                        */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "datoviz/geom.h"

#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include <vector>

#include "_alloc.h"
#include "_log.h"
#include "_overflow.h"
#include "earcut.hpp"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

static const double DVZ_POLYGON_AREA_EPSILON = 1e-20;



/*************************************************************************************************/
/*  Types                                                                                        */
/*************************************************************************************************/

typedef std::array<double, 2> _DvzEarcutPoint;
typedef std::vector<_DvzEarcutPoint> _DvzEarcutRing;
typedef std::vector<_DvzEarcutRing> _DvzEarcutPolygon;



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Return whether a 2D point has finite coordinates.
 *
 * @param xy point coordinates
 * @return whether both coordinates are finite
 */
static bool _polygon_point_finite(const dvec2 xy)
{
    return xy != NULL && std::isfinite(xy[0]) && std::isfinite(xy[1]);
}



/**
 * Return whether two 2D points are exactly equal.
 *
 * @param a first point
 * @param b second point
 * @return whether the points have equal coordinates
 */
static bool _polygon_points_equal(const dvec2 a, const dvec2 b)
{
    return a != NULL && b != NULL && a[0] == b[0] && a[1] == b[1];
}



/**
 * Return the number of vertices after dropping an optional repeated closing point.
 *
 * @param ring borrowed polygon ring
 * @return normalized vertex count
 */
static uint32_t _polygon_normalized_count(const DvzPolygonRing* ring)
{
    if (ring == NULL || ring->xy == NULL || ring->count == 0)
        return 0;

    uint32_t count = ring->count;
    if (count >= 2 && _polygon_points_equal(ring->xy[0], ring->xy[count - 1]))
        count--;
    return count;
}



/**
 * Compute the signed area of a normalized ring.
 *
 * @param ring borrowed polygon ring
 * @param count normalized vertex count
 * @return signed ring area
 */
static double _polygon_ring_area(const DvzPolygonRing* ring, uint32_t count)
{
    double area = 0.0;
    for (uint32_t i = 0; i < count; i++)
    {
        const uint32_t j = (i + 1) % count;
        area += ring->xy[i][0] * ring->xy[j][1] - ring->xy[j][0] * ring->xy[i][1];
    }
    return 0.5 * area;
}



/**
 * Validate one polygon ring and return its normalized vertex count.
 *
 * @param ring borrowed polygon ring
 * @param normalized_count output normalized vertex count
 * @return whether the ring is valid
 */
static bool _polygon_validate_ring(const DvzPolygonRing* ring, uint32_t* normalized_count)
{
    if (normalized_count == NULL)
        return false;
    *normalized_count = 0;

    if (ring == NULL || ring->xy == NULL)
        return false;

    const uint32_t count = _polygon_normalized_count(ring);
    if (count < 3)
        return false;

    for (uint32_t i = 0; i < count; i++)
    {
        if (!_polygon_point_finite(ring->xy[i]))
            return false;
    }

    if (std::fabs(_polygon_ring_area(ring, count)) <= DVZ_POLYGON_AREA_EPSILON)
        return false;

    *normalized_count = count;
    return true;
}



/**
 * Append a validated ring to an Earcut polygon and to the output geometry vertex stream.
 *
 * @param ring borrowed polygon ring
 * @param count normalized vertex count
 * @param polygon Earcut polygon
 * @param geometry output geometry
 * @param vertex_offset first output vertex index
 */
static void _polygon_append_ring(
    const DvzPolygonRing* ring, uint32_t count, _DvzEarcutPolygon& polygon, DvzGeometry* geometry,
    uint32_t vertex_offset)
{
    _DvzEarcutRing out_ring;
    out_ring.reserve(count);

    for (uint32_t i = 0; i < count; i++)
    {
        const double x = ring->xy[i][0];
        const double y = ring->xy[i][1];
        out_ring.push_back({x, y});

        const uint32_t vertex = vertex_offset + i;
        geometry->positions[vertex][0] = x;
        geometry->positions[vertex][1] = y;
        geometry->positions[vertex][2] = 0.0;

        geometry->normals[vertex][0] = 0.0;
        geometry->normals[vertex][1] = 0.0;
        geometry->normals[vertex][2] = 1.0;

        geometry->texcoords[vertex][0] = 0.0;
        geometry->texcoords[vertex][1] = 0.0;

        geometry->colors[vertex] = dvz_color_rgb(255, 255, 255);
    }

    polygon.push_back(out_ring);
}



/**
 * Return whether the triangulation backend descriptor is supported.
 *
 * @param desc optional triangulation descriptor
 * @return whether the backend is supported
 */
static bool _triangulation_backend_supported(const DvzTriangulationDesc* desc)
{
    if (desc == NULL)
        return true;
    return desc->backend == DVZ_TRIANGULATION_BACKEND_DEFAULT ||
           desc->backend == DVZ_TRIANGULATION_BACKEND_EARCUT;
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Triangulate a polygon with optional holes into indexed XY mesh geometry.
 *
 * @param polygon borrowed polygon descriptor
 * @param desc optional triangulation descriptor
 * @return the triangulated geometry, or NULL on invalid input or triangulation failure
 */
DvzGeometry* dvz_triangulate_polygon(
    const DvzPolygonDesc* polygon, const DvzTriangulationDesc* desc)
{
    try
    {
        if (polygon == NULL || !_triangulation_backend_supported(desc))
            return NULL;

        if (polygon->hole_count > 0 && polygon->holes == NULL)
            return NULL;

        uint32_t outer_count = 0;
        if (!_polygon_validate_ring(&polygon->outer, &outer_count))
            return NULL;

        std::vector<uint32_t> ring_counts;
        ring_counts.reserve((size_t)polygon->hole_count + 1);
        ring_counts.push_back(outer_count);

        uint64_t total_vertices = outer_count;
        for (uint32_t i = 0; i < polygon->hole_count; i++)
        {
            uint32_t hole_count = 0;
            if (!_polygon_validate_ring(&polygon->holes[i], &hole_count))
                return NULL;
            if (_dvz_add_u64_overflows(total_vertices, hole_count, &total_vertices))
                return NULL;
            if (total_vertices > UINT32_MAX ||
                total_vertices > (uint64_t)std::numeric_limits<DvzIndex>::max())
                return NULL;
            ring_counts.push_back(hole_count);
        }

        const uint32_t vertex_count = (uint32_t)total_vertices;
        DvzGeometry* geometry = dvz_geometry(vertex_count, 0);
        if (geometry == NULL)
            return NULL;

        _DvzEarcutPolygon earcut_polygon;
        earcut_polygon.reserve((size_t)polygon->hole_count + 1);

        uint32_t vertex_offset = 0;
        _polygon_append_ring(&polygon->outer, ring_counts[0], earcut_polygon, geometry, vertex_offset);
        vertex_offset += ring_counts[0];
        for (uint32_t i = 0; i < polygon->hole_count; i++)
        {
            _polygon_append_ring(
                &polygon->holes[i], ring_counts[(size_t)i + 1], earcut_polygon, geometry,
                vertex_offset);
            vertex_offset += ring_counts[(size_t)i + 1];
        }

        const std::vector<DvzIndex> indices = mapbox::earcut<DvzIndex>(earcut_polygon);
        if (indices.empty() || indices.size() % 3 != 0 || indices.size() > UINT32_MAX)
        {
            dvz_geometry_destroy(geometry);
            return NULL;
        }

        if (_dvz_mul_u64_overflows((uint64_t)indices.size(), sizeof(DvzIndex), &total_vertices))
        {
            dvz_geometry_destroy(geometry);
            return NULL;
        }

        geometry->indices = (DvzIndex*)dvz_calloc((DvzSize)indices.size(), sizeof(DvzIndex));
        if (geometry->indices == NULL)
        {
            dvz_geometry_destroy(geometry);
            return NULL;
        }

        geometry->index_count = (uint32_t)indices.size();
        for (uint32_t i = 0; i < geometry->index_count; i++)
        {
            if (indices[i] >= vertex_count)
            {
                dvz_geometry_destroy(geometry);
                return NULL;
            }
            geometry->indices[i] = indices[i];
        }

        geometry->type = DVZ_GEOMETRY_CUSTOM;
        geometry->flags = DVZ_GEOMETRY_INDEXING_TRIANGLES | DVZ_GEOMETRY_INDEXING_TRIANGULATION;
        return geometry;
    }
    catch (...)
    {
        log_error("polygon triangulation failed");
        return NULL;
    }
}
