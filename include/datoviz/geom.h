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
#include "datoviz/common/types.h"



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
 * The returned object owns arrays for @p vertex_count vertices and @p index_count indices. Destroy
 * it with `dvz_geometry_destroy()`.
 *
 * @param vertex_count number of vertices to allocate; may be zero for an empty geometry
 * @param index_count number of indices to allocate; may be zero for non-indexed geometry
 * @return new owned geometry, or NULL on invalid input or allocation failure
 */
DVZ_EXPORT DvzGeometry* dvz_geometry(uint32_t vertex_count, uint32_t index_count);



/**
 * Free all buffers owned by a geometry object and reset it to an empty state.
 *
 * @param geometry geometry to reset; must not be NULL
 * @return DVZ_OK on success, DVZ_ERROR on validation error
 */
DVZ_EXPORT DvzResult dvz_geometry_reset(DvzGeometry* geometry);



/**
 * Destroy a geometry object.
 *
 * @param geometry owned geometry to destroy; may be NULL
 */
DVZ_EXPORT void dvz_geometry_destroy(DvzGeometry* geometry);



/**
 * Compute the bounds of a geometry object's positions.
 *
 * @param geometry geometry to inspect; must not be NULL
 * @return the geometry bounds, or an empty zero bounds when no vertices exist
 */
DVZ_EXPORT DvzGeometryBounds dvz_geometry_bounds(const DvzGeometry* geometry);



/**
 * Recompute smooth vertex normals from triangle indices.
 *
 * @param geometry indexed triangle geometry to update; must not be NULL
 * @return DVZ_OK on success, DVZ_ERROR on invalid input
 */
DVZ_EXPORT DvzResult dvz_geometry_compute_normals(DvzGeometry* geometry);



/**
 * Apply an affine transform to positions and normals in place.
 *
 * @param geometry geometry to update in place; must not be NULL
 * @param transform affine 4x4 transform matrix
 * @return DVZ_OK on success, DVZ_ERROR on invalid input
 */
DVZ_EXPORT DvzResult dvz_geometry_transform(DvzGeometry* geometry, dmat4 transform);



/**
 * Merge several geometry objects into one indexed geometry.
 *
 * @param count number of input geometry pointers; must be positive
 * @param geometries array of @p count borrowed geometry pointers; entries must not be NULL
 * @return new owned merged geometry, or NULL on invalid input or allocation failure; destroy with
 * `dvz_geometry_destroy()`
 */
DVZ_EXPORT DvzGeometry*
dvz_geometry_merge(uint32_t count, const DvzGeometry* const* geometries);



/**
 * Derive a unique edge list from indexed triangle geometry.
 *
 * @param geometry borrowed indexed triangle geometry; must not be NULL
 * @return new owned edge list, or NULL on invalid input or allocation failure; destroy with
 * `dvz_geometry_edges_destroy()`
 */
DVZ_EXPORT DvzGeometryEdges* dvz_geometry_edges(const DvzGeometry* geometry);



/**
 * Destroy a derived geometry edge list.
 *
 * @param edges owned edge list to destroy; may be NULL
 */
DVZ_EXPORT void dvz_geometry_edges_destroy(DvzGeometryEdges* edges);



/**
 * Extract contour line segments from indexed triangle geometry and per-vertex scalar values.
 *
 * @param geometry borrowed indexed triangle geometry; must not be NULL
 * @param values array containing one scalar per geometry vertex; must not be NULL
 * @param value_count number of values; must equal the geometry vertex count
 * @param levels array of contour levels; must not be NULL
 * @param level_count number of contour levels; must be positive
 * @return new owned contour segments, or NULL on invalid input or allocation failure; destroy with
 * `dvz_geometry_contours_destroy()`
 */
DVZ_EXPORT DvzGeometryContours* dvz_geometry_contours(
    const DvzGeometry* geometry, const double* values, uint32_t value_count, const double* levels,
    uint32_t level_count);



/**
 * Destroy extracted contour segments.
 *
 * @param contours owned contour segment list to destroy; may be NULL
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
 * @param desc optional borrowed cube descriptor, or NULL for defaults
 * @return new owned geometry, or NULL on failure; destroy with `dvz_geometry_destroy()`
 */
DVZ_EXPORT DvzGeometry* dvz_geometry_cube(const DvzGeometryCubeDesc* desc);



/**
 * Return a default plane geometry descriptor.
 *
 * @return initialized plane descriptor
 */
DVZ_EXPORT DvzGeometryPlaneDesc dvz_geometry_plane_desc(void);


/**
 * Create an indexed XY plane geometry.
 *
 * @param desc optional borrowed plane descriptor, or NULL for defaults
 * @return new owned geometry, or NULL on failure; destroy with `dvz_geometry_destroy()`
 */
DVZ_EXPORT DvzGeometry* dvz_geometry_plane(const DvzGeometryPlaneDesc* desc);



/**
 * Return a default sphere geometry descriptor.
 *
 * @return initialized sphere descriptor
 */
DVZ_EXPORT DvzGeometrySphereDesc dvz_geometry_sphere_desc(void);


/**
 * Create an indexed UV-sphere geometry.
 *
 * @param desc optional borrowed sphere descriptor, or NULL for defaults
 * @return new owned geometry, or NULL on failure; destroy with `dvz_geometry_destroy()`
 */
DVZ_EXPORT DvzGeometry* dvz_geometry_sphere(const DvzGeometrySphereDesc* desc);



/**
 * Return a default surface-grid geometry descriptor.
 *
 * @return initialized surface-grid descriptor
 */
DVZ_EXPORT DvzGeometrySurfaceGridDesc dvz_geometry_surface_grid_desc(void);


/**
 * Create an indexed structured surface-grid geometry.
 *
 * @param desc borrowed surface-grid descriptor; must not be NULL
 * @return new owned geometry, or NULL on failure; destroy with `dvz_geometry_destroy()`
 */
DVZ_EXPORT DvzGeometry* dvz_geometry_surface_grid(const DvzGeometrySurfaceGridDesc* desc);



/**
 * Update the heights of an existing structured surface-grid geometry.
 *
 * @param geometry surface-grid geometry to update; must not be NULL
 * @param heights row-major height array; must not be NULL
 * @param count number of height values; must equal the grid vertex count
 * @return DVZ_OK on success, DVZ_ERROR on invalid input
 */
DVZ_EXPORT DvzResult
dvz_geometry_surface_grid_update_heights(
    DvzGeometry* geometry, const double* heights, uint32_t count);


/**
 * Return a default disc geometry descriptor.
 *
 * @return initialized disc descriptor
 */
DVZ_EXPORT DvzGeometryDiscDesc dvz_geometry_disc_desc(void);


/**
 * Create an indexed XY disc geometry.
 *
 * @param desc optional borrowed disc descriptor, or NULL for defaults
 * @return new owned geometry, or NULL on failure; destroy with `dvz_geometry_destroy()`
 */
DVZ_EXPORT DvzGeometry* dvz_geometry_disc(const DvzGeometryDiscDesc* desc);


/**
 * Return a default sector geometry descriptor.
 *
 * @return initialized sector descriptor
 */
DVZ_EXPORT DvzGeometrySectorDesc dvz_geometry_sector_desc(void);


/**
 * Create an indexed XY sector geometry.
 *
 * @param desc optional borrowed sector descriptor, or NULL for defaults
 * @return new owned geometry, or NULL on failure; destroy with `dvz_geometry_destroy()`
 */
DVZ_EXPORT DvzGeometry* dvz_geometry_sector(const DvzGeometrySectorDesc* desc);


/**
 * Return a default regular-polygon geometry descriptor.
 *
 * @return initialized regular-polygon descriptor
 */
DVZ_EXPORT DvzGeometryRegularPolygonDesc dvz_geometry_regular_polygon_desc(void);


/**
 * Create an indexed XY regular-polygon geometry.
 *
 * @param desc optional borrowed regular-polygon descriptor, or NULL for defaults
 * @return new owned geometry, or NULL on failure; destroy with `dvz_geometry_destroy()`
 */
DVZ_EXPORT DvzGeometry* dvz_geometry_regular_polygon(const DvzGeometryRegularPolygonDesc* desc);


/**
 * Return a default star geometry descriptor.
 *
 * @return initialized star descriptor
 */
DVZ_EXPORT DvzGeometryStarDesc dvz_geometry_star_desc(void);


/**
 * Create an indexed XY star geometry.
 *
 * @param desc optional borrowed star descriptor, or NULL for defaults
 * @return new owned geometry, or NULL on failure; destroy with `dvz_geometry_destroy()`
 */
DVZ_EXPORT DvzGeometry* dvz_geometry_star(const DvzGeometryStarDesc* desc);


/**
 * Return a default cylinder geometry descriptor.
 *
 * @return initialized cylinder descriptor
 */
DVZ_EXPORT DvzGeometryCylinderDesc dvz_geometry_cylinder_desc(void);


/**
 * Create an indexed Z-axis cylinder geometry.
 *
 * @param desc optional borrowed cylinder descriptor, or NULL for defaults
 * @return new owned geometry, or NULL on failure; destroy with `dvz_geometry_destroy()`
 */
DVZ_EXPORT DvzGeometry* dvz_geometry_cylinder(const DvzGeometryCylinderDesc* desc);


/**
 * Return a default cone geometry descriptor.
 *
 * @return initialized cone descriptor
 */
DVZ_EXPORT DvzGeometryConeDesc dvz_geometry_cone_desc(void);


/**
 * Create an indexed Z-axis cone geometry.
 *
 * @param desc optional borrowed cone descriptor, or NULL for defaults
 * @return new owned geometry, or NULL on failure; destroy with `dvz_geometry_destroy()`
 */
DVZ_EXPORT DvzGeometry* dvz_geometry_cone(const DvzGeometryConeDesc* desc);


/**
 * Return a default torus geometry descriptor.
 *
 * @return initialized torus descriptor
 */
DVZ_EXPORT DvzGeometryTorusDesc dvz_geometry_torus_desc(void);


/**
 * Create an indexed torus geometry around the Z axis.
 *
 * @param desc optional borrowed torus descriptor, or NULL for defaults
 * @return new owned geometry, or NULL on failure; destroy with `dvz_geometry_destroy()`
 */
DVZ_EXPORT DvzGeometry* dvz_geometry_torus(const DvzGeometryTorusDesc* desc);


/**
 * Return a default arrow geometry descriptor.
 *
 * @return initialized arrow descriptor
 */
DVZ_EXPORT DvzGeometryArrowDesc dvz_geometry_arrow_desc(void);


/**
 * Create an indexed Z-axis arrow geometry.
 *
 * @param desc optional borrowed arrow descriptor, or NULL for defaults
 * @return new owned geometry, or NULL on failure; destroy with `dvz_geometry_destroy()`
 */
DVZ_EXPORT DvzGeometry* dvz_geometry_arrow(const DvzGeometryArrowDesc* desc);


/**
 * Return a default Wavefront OBJ geometry loader descriptor.
 *
 * @return initialized OBJ loader descriptor
 */
DVZ_EXPORT DvzGeometryObjDesc dvz_geometry_obj_desc(void);


/**
 * Load a Wavefront OBJ mesh as indexed geometry.
 *
 * The first loader slice supports `v`, `vn`, and polygonal `f` records. Faces are triangulated as
 * fans and texture coordinates/materials are ignored.
 *
 * @param filename OBJ file path; must not be NULL
 * @param desc optional borrowed loader descriptor, or NULL for defaults
 * @return new owned geometry, or NULL on unsupported input, allocation, or I/O failure; destroy
 * with `dvz_geometry_destroy()`
 */
DVZ_EXPORT DvzGeometry* dvz_geometry_obj(const char* filename, const DvzGeometryObjDesc* desc);



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
 * @param polygon borrowed polygon descriptor; must not be NULL and its arrays must remain valid for
 * the duration of the call
 * @param desc optional borrowed triangulation descriptor, or NULL for defaults
 * @return new owned geometry, or NULL on invalid input or triangulation failure; destroy with
 * `dvz_geometry_destroy()`
 */
DVZ_EXPORT DvzGeometry*
dvz_triangulate_polygon(const DvzPolygonDesc* polygon, const DvzTriangulationDesc* desc);


/**
 * Tessellate a quadratic Bezier curve into an owned point path.
 *
 * @param p0 first endpoint
 * @param p1 control point
 * @param p2 second endpoint
 * @param desc optional borrowed tessellation descriptor, or NULL for defaults
 * @return new owned tessellated path, or NULL on invalid input or allocation failure; destroy with
 * `dvz_tessellated_path_destroy()`
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
 * @param desc optional borrowed tessellation descriptor, or NULL for defaults
 * @return new owned tessellated path, or NULL on invalid input or allocation failure; destroy with
 * `dvz_tessellated_path_destroy()`
 */
DVZ_EXPORT DvzTessellatedPath* dvz_tessellate_cubic_bezier(
    const dvec3 p0, const dvec3 p1, const dvec3 p2, const dvec3 p3,
    const DvzBezierTessellationDesc* desc);


/**
 * Destroy a tessellated path.
 *
 * @param path owned tessellated path to destroy; may be NULL
 */
DVZ_EXPORT void dvz_tessellated_path_destroy(DvzTessellatedPath* path);

EXTERN_C_OFF
