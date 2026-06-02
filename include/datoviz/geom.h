/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Geometry                                                                                     */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "geom/enums.h"
#include "geom/types.h"
#include "datoviz/common/macros.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define DVZ_GEOM_CUBE_FACE_COUNT 6



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

EXTERN_C_ON

/**
 * Allocate a geometry object with owned vertex and index buffers.
 *
 * @param vertex_count number of vertices
 * @param index_count number of indices
 * @return the new geometry, or NULL on failure
 */
DVZ_EXPORT DvzGeometry* dvz_geometry(uint32_t vertex_count, uint32_t index_count);



/**
 * Free all buffers owned by a geometry object and reset it to an empty state.
 *
 * @param geometry the geometry
 */
DVZ_EXPORT void dvz_geometry_reset(DvzGeometry* geometry);



/**
 * Destroy a geometry object.
 *
 * @param geometry the geometry
 */
DVZ_EXPORT void dvz_geometry_destroy(DvzGeometry* geometry);



/**
 * Compute the bounds of a geometry object's positions.
 *
 * @param geometry the geometry
 * @return the geometry bounds, or an empty zero bounds when no vertices exist
 */
DVZ_EXPORT DvzGeometryBounds dvz_geometry_bounds(const DvzGeometry* geometry);



/**
 * Recompute smooth vertex normals from triangle indices.
 *
 * @param geometry the geometry
 * @return 0 on success, -1 on invalid input
 */
DVZ_EXPORT int dvz_geometry_compute_normals(DvzGeometry* geometry);



/**
 * Apply an affine transform to positions and normals in place.
 *
 * @param geometry the geometry
 * @param transform affine transform matrix
 * @return 0 on success, -1 on invalid input
 */
DVZ_EXPORT int dvz_geometry_transform(DvzGeometry* geometry, dmat4 transform);



/**
 * Merge several geometry objects into one indexed geometry.
 *
 * @param count number of input geometries
 * @param geometries input geometry array
 * @return the merged geometry, or NULL on failure
 */
DVZ_EXPORT DvzGeometry*
dvz_geometry_merge(uint32_t count, const DvzGeometry* const* geometries);



/**
 * Derive a unique edge list from indexed triangle geometry.
 *
 * @param geometry the geometry
 * @return the derived edge list, or NULL on invalid input or allocation failure
 */
DVZ_EXPORT DvzGeometryEdges* dvz_geometry_edges(const DvzGeometry* geometry);



/**
 * Destroy a derived geometry edge list.
 *
 * @param edges the edge list
 */
DVZ_EXPORT void dvz_geometry_edges_destroy(DvzGeometryEdges* edges);



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
DVZ_EXPORT DvzGeometryContours* dvz_geometry_contours(
    const DvzGeometry* geometry, const double* values, uint32_t value_count, const double* levels,
    uint32_t level_count);



/**
 * Destroy extracted contour segments.
 *
 * @param contours the contour segment list
 */
DVZ_EXPORT void dvz_geometry_contours_destroy(DvzGeometryContours* contours);



/**
 * Return a default cube geometry descriptor.
 *
 * @return initialized cube descriptor
 */
DVZ_EXPORT DvzGeometryCubeDesc dvz_geometry_cube_desc(void);


/**
 * Create an indexed cube geometry.
 *
 * @param desc optional cube descriptor
 * @return the new geometry, or NULL on failure
 */
DVZ_EXPORT DvzGeometry* dvz_geom_cube(const DvzGeometryCubeDesc* desc);



/**
 * Return a default plane geometry descriptor.
 *
 * @return initialized plane descriptor
 */
DVZ_EXPORT DvzGeometryPlaneDesc dvz_geometry_plane_desc(void);


/**
 * Create an indexed XY plane geometry.
 *
 * @param desc optional plane descriptor
 * @return the new geometry, or NULL on failure
 */
DVZ_EXPORT DvzGeometry* dvz_geom_plane(const DvzGeometryPlaneDesc* desc);



/**
 * Return a default sphere geometry descriptor.
 *
 * @return initialized sphere descriptor
 */
DVZ_EXPORT DvzGeometrySphereDesc dvz_geometry_sphere_desc(void);


/**
 * Create an indexed UV-sphere geometry.
 *
 * @param desc optional sphere descriptor
 * @return the new geometry, or NULL on failure
 */
DVZ_EXPORT DvzGeometry* dvz_geom_sphere(const DvzGeometrySphereDesc* desc);



/**
 * Return a default surface-grid geometry descriptor.
 *
 * @return initialized surface-grid descriptor
 */
DVZ_EXPORT DvzGeometrySurfaceGridDesc dvz_geometry_surface_grid_desc(void);


/**
 * Create an indexed structured surface-grid geometry.
 *
 * @param desc surface-grid descriptor
 * @return the new geometry, or NULL on failure
 */
DVZ_EXPORT DvzGeometry* dvz_geom_surface_grid(const DvzGeometrySurfaceGridDesc* desc);



/**
 * Update the heights of an existing structured surface-grid geometry.
 *
 * @param geometry the surface-grid geometry
 * @param heights row-major height values
 * @param count number of height values
 * @return 0 on success, -1 on invalid input
 */
DVZ_EXPORT int
dvz_geom_surface_grid_update_heights(DvzGeometry* geometry, const double* heights, uint32_t count);



/**
 * Return a default polygon descriptor.
 *
 * @return initialized polygon descriptor
 */
DVZ_EXPORT DvzPolygonDesc dvz_polygon_desc(void);


/**
 * Return a default triangulation descriptor.
 *
 * @return initialized triangulation descriptor
 */
DVZ_EXPORT DvzTriangulationDesc dvz_triangulation_desc(void);


/**
 * Return a default Bezier tessellation descriptor.
 *
 * @return initialized Bezier tessellation descriptor
 */
DVZ_EXPORT DvzBezierTessellationDesc dvz_bezier_tessellation_desc(void);


/**
 * Triangulate a polygon with optional holes into indexed XY mesh geometry.
 *
 * @param polygon borrowed polygon descriptor
 * @param desc optional triangulation descriptor
 * @return the triangulated geometry, or NULL on invalid input or triangulation failure
 */
DVZ_EXPORT DvzGeometry*
dvz_triangulate_polygon(const DvzPolygonDesc* polygon, const DvzTriangulationDesc* desc);


/**
 * Tessellate a quadratic Bezier curve into an owned point path.
 *
 * @param p0 first endpoint
 * @param p1 control point
 * @param p2 second endpoint
 * @param desc optional tessellation descriptor
 * @return the tessellated path, or NULL on invalid input or allocation failure
 */
DVZ_EXPORT DvzTessellatedPath* dvz_tessellate_quadratic_bezier(
    const dvec3 p0, const dvec3 p1, const dvec3 p2, const DvzBezierTessellationDesc* desc);


/**
 * Tessellate a cubic Bezier curve into an owned point path.
 *
 * @param p0 first endpoint
 * @param p1 first control point
 * @param p2 second control point
 * @param p3 second endpoint
 * @param desc optional tessellation descriptor
 * @return the tessellated path, or NULL on invalid input or allocation failure
 */
DVZ_EXPORT DvzTessellatedPath* dvz_tessellate_cubic_bezier(
    const dvec3 p0, const dvec3 p1, const dvec3 p2, const dvec3 p3,
    const DvzBezierTessellationDesc* desc);


/**
 * Destroy a tessellated path.
 *
 * @param path tessellated path
 */
DVZ_EXPORT void dvz_tessellated_path_destroy(DvzTessellatedPath* path);

EXTERN_C_OFF
