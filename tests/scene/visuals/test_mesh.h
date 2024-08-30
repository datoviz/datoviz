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

int test_mesh_1(TstSuite*);

int test_mesh_polygon(TstSuite*);

int test_mesh_stroke(TstSuite*);

int test_mesh_contour(TstSuite*);

int test_mesh_surface(TstSuite*);

int test_mesh_obj(TstSuite*);

int test_mesh_geo(TstSuite* suite);



#endif
