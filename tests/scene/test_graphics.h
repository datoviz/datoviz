/*
* Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
* Licensed under the MIT license. See LICENSE file in the project root for details.
* SPDX-License-Identifier: MIT
*/

/*************************************************************************************************/
/*  Tests                                                                                        */
/*************************************************************************************************/

#ifndef DVZ_HEADER_TEST_GRAPHICS
#define DVZ_HEADER_TEST_GRAPHICS



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "testing.h"



/*************************************************************************************************/
/*  Graphics tests                                                                               */
/*************************************************************************************************/

int test_graphics_point(TstSuite*);

int test_graphics_triangle(TstSuite*);

int test_graphics_line_list(TstSuite*);

int test_graphics_line_strip(TstSuite*);

int test_graphics_triangle_list(TstSuite*);

int test_graphics_triangle_strip(TstSuite*);

int test_graphics_triangle_fan(TstSuite*);

int test_graphics_raster(TstSuite*);

int test_graphics_marker(TstSuite*);

int test_graphics_segment(TstSuite*);

int test_graphics_path(TstSuite*);

int test_graphics_text(TstSuite*);

int test_graphics_image_1(TstSuite*);

int test_graphics_image_cmap(TstSuite*);

int test_graphics_volume_slice(TstSuite*);

int test_graphics_volume_1(TstSuite*);

int test_graphics_mesh(TstSuite*);



#endif
