/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Testing geometry                                                                             */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "testing.h"



/*************************************************************************************************/
/*  Geometry tests                                                                               */
/*************************************************************************************************/

int test_geometry_alloc(TstContext* suite, const TstCase* tstitem);

int test_geometry_descriptor_abi(TstContext* suite, const TstCase* tstitem);

int test_geometry_cube(TstContext* suite, const TstCase* tstitem);

int test_geometry_plane(TstContext* suite, const TstCase* tstitem);

int test_geometry_surface_grid(TstContext* suite, const TstCase* tstitem);

int test_geometry_surface_grid_update(TstContext* suite, const TstCase* tstitem);

int test_geometry_sphere(TstContext* suite, const TstCase* tstitem);

int test_geometry_builtin_shapes(TstContext* suite, const TstCase* tstitem);

int test_geometry_transform(TstContext* suite, const TstCase* tstitem);

int test_geometry_merge(TstContext* suite, const TstCase* tstitem);

int test_geometry_edges(TstContext* suite, const TstCase* tstitem);

int test_geometry_contours(TstContext* suite, const TstCase* tstitem);

int test_geometry_polygon_triangulation(TstContext* suite, const TstCase* tstitem);

int test_geometry_polygon_triangulation_invalid(TstContext* suite, const TstCase* tstitem);

int test_geometry_bezier_tessellation(TstContext* suite, const TstCase* tstitem);



/*************************************************************************************************/
/*  Geometry test entry-point                                                                    */
/*************************************************************************************************/

int test_geom(TstSuite* suite);
