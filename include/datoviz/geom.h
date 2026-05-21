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
 * @return the geometry bounds, or an empty zero box when no vertices exist
 */
DVZ_EXPORT DvzBox dvz_geometry_bounds(const DvzGeometry* geometry);



/**
 * Convert geometry positions to F32 vectors for current scene mesh upload paths.
 *
 * @param geometry the geometry
 * @param out output F32 position buffer
 * @param out_count number of vectors available in out
 * @return 0 on success, -1 on invalid input
 */
DVZ_EXPORT int dvz_geometry_positions_f32(
    const DvzGeometry* geometry, vec3* out, uint32_t out_count);



/**
 * Convert geometry normals to F32 vectors for current scene mesh upload paths.
 *
 * @param geometry the geometry
 * @param out output F32 normal buffer
 * @param out_count number of vectors available in out
 * @return 0 on success, -1 on invalid input
 */
DVZ_EXPORT int dvz_geometry_normals_f32(
    const DvzGeometry* geometry, vec3* out, uint32_t out_count);



/**
 * Create an indexed cube geometry.
 *
 * @param desc optional cube descriptor
 * @return the new geometry, or NULL on failure
 */
DVZ_EXPORT DvzGeometry* dvz_geom_cube(const DvzGeometryCubeDesc* desc);



/**
 * Create an indexed XY plane geometry.
 *
 * @param desc optional plane descriptor
 * @return the new geometry, or NULL on failure
 */
DVZ_EXPORT DvzGeometry* dvz_geom_plane(const DvzGeometryPlaneDesc* desc);

EXTERN_C_OFF
