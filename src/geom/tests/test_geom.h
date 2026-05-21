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

int test_geometry_cube(TstContext* suite, const TstCase* tstitem);

int test_geometry_plane(TstContext* suite, const TstCase* tstitem);

int test_geometry_f32(TstContext* suite, const TstCase* tstitem);



/*************************************************************************************************/
/*  Geometry test entry-point                                                                    */
/*************************************************************************************************/

int test_geom(TstSuite* suite);
