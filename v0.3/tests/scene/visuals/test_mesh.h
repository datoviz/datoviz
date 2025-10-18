/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Tests                                                                                        */
/*************************************************************************************************/

#ifndef DVZ_HEADER_TEST_MESH
#define DVZ_HEADER_TEST_MESH



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "testing.h"



/*************************************************************************************************/
/*  Pixel tests                                                                                  */
/*************************************************************************************************/

int test_mesh_1(TstSuite* suite, TstItem* tstitem);

int test_mesh_2(TstSuite* suite, TstItem* tstitem);

int test_mesh_polygon(TstSuite* suite, TstItem* tstitem);

int test_mesh_edgecolor(TstSuite* suite, TstItem* tstitem);

int test_mesh_contour(TstSuite* suite, TstItem* tstitem);

int test_mesh_surface(TstSuite* suite, TstItem* tstitem);

int test_mesh_obj(TstSuite* suite, TstItem* tstitem);

int test_mesh_geo(TstSuite* suite, TstItem* tstitem);



#endif
