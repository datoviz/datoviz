/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Testing box                                                                                  */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "test_box.h"
#include "scene/box.h"
#include "test.h"
#include "testing.h"
#include "testing_utils.h"



/*************************************************************************************************/
/* Box tests                                                                                     */
/*************************************************************************************************/

int test_box_1(TstSuite* suite)
{
    ANN(suite);

    // Test dvz_box creation
    DvzBox box = dvz_box(-10.0, 10.0, -5.0, 5.0, 0, 2);
    AT(box.xmin == -10.0);
    AT(box.xmax == 10.0);
    AT(box.ymin == -5.0);
    AT(box.ymax == 5.0);
    AT(box.zmin == 0);
    AT(box.zmax == 2);

    return 0;
}



int test_box_2(TstSuite* suite)
{
    ANN(suite);

    // Test dvz_box_extent with fixed aspect ratio expand
    DvzBox box = dvz_box(-1.0, 1.0, -1.0, 1.0, -1.0, 1.0);
    DvzBox result = dvz_box_extent(box, 4.0, 3.0, DVZ_BOX_EXTENT_FIXED_ASPECT_EXPAND);

    // Check that aspect ratio is 4:3
    AC(dvz_box_aspect(result), 4.0 / 3.0, EPS);

    // Ensure the box contains the original
    AT(result.xmin <= box.xmin);
    AT(result.xmax >= box.xmax);
    AT(result.ymin <= box.ymin);
    AT(result.ymax >= box.ymax);

    // Ensure the box center is correct.
    dvec3 center = {0};
    dvz_box_center(result, center);
    AC(center[0], 0, EPS);
    AC(center[1], 0, EPS);
    AC(center[2], 0, EPS);

    return 0;
}



int test_box_3(TstSuite* suite)
{
    ANN(suite);

    // Test dvz_box_merge
    DvzBox boxes[2] = {
        dvz_box(-1.0, 1.0, -1.0, 1.0, -1.0, 1.0),
        dvz_box(-2.0, 2.0, -0.5, 0.5, -1.0, 1.0),
    };
    DvzBox merged = dvz_box_merge(2, boxes, DVZ_BOX_MERGE_DEFAULT);

    AT(merged.xmin == -2.0);
    AT(merged.xmax == 2.0);
    AT(merged.ymin == -1.0);
    AT(merged.ymax == 1.0);
    AT(merged.zmin == -1.0);
    AT(merged.zmax == 1.0);

    return 0;
}



int test_box_4(TstSuite* suite)
{
    ANN(suite);

    // Test dvz_box_normalize
    DvzBox source = dvz_box(0.0, 10.0, 0.0, 10.0, 0.0, 10.0);
    DvzBox target = dvz_box(-1.0, 1.0, -1.0, 1.0, -1.0, 1.0);
    dvec3 pos[2] = {{5.0, 5.0, 5.0}, {10.0, 10.0, 10.0}};
    vec3 out[2];
    dvz_box_normalize(source, target, 2, pos, out);

    // Check normalized positions
    vec3 expected[2] = {{0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}};
    ACn(3, out[0], expected[0], EPS);
    ACn(3, out[1], expected[1], EPS);

    return 0;
}



int test_box_5(TstSuite* suite)
{
    ANN(suite);

    // Test dvz_box_inverse
    DvzBox source = dvz_box(0.0, 10.0, 0.0, 10.0, 0.0, 10.0);
    DvzBox target = dvz_box(-1.0, 1.0, -1.0, 1.0, -1.0, 1.0);
    vec3 pos = {0.0f, 0.0f, 0.0f};
    dvec3 out;
    dvz_box_inverse(source, target, pos, &out);

    // Check the inverse transform
    dvec3 expected = {5.0, 5.0, 5.0};
    ACn(3, out, expected, EPS);

    return 0;
}



int test_box_6(TstSuite* suite)
{
    ANN(suite);

    // Test dvz_box_normalize_2D
    DvzBox source = dvz_box(0.0, 10.0, 0.0, 10.0, 0.0, 10.0);
    DvzBox target = dvz_box(-1.0, 1.0, -1.0, 1.0, -1.0, 1.0);
    dvec2 pos[2] = {{5.0, 5.0}, {10.0, 10.0}};
    vec3 out[2];
    dvz_box_normalize_2D(source, target, 2, pos, out);

    // Check normalized positions
    vec3 expected[2] = {{0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 0.0f}};
    ACn(2, out[0], expected[0], EPS);
    ACn(2, out[1], expected[1], EPS);

    return 0;
}
