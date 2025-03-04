/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Testing shape                                                                                */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "test_shape.h"
#include "datoviz.h"
#include "test.h"
#include "testing.h"
#include "testing_utils.h"



/*************************************************************************************************/
/*  Shape tests                                                                                  */
/*************************************************************************************************/

int test_shape_1(TstSuite* suite, TstItem* tstitem)
{
    ANN(suite);

    DvzColor color = {RED};
    const uint32_t count = 30;

    DvzShape* square = dvz_shape();
    dvz_shape_square(square, color);
    DvzShape* disc = dvz_shape();
    dvz_shape_disc(disc, count, color);

    dvz_shape_destroy(disc);
    dvz_shape_destroy(square);
    return 0;
}



int test_shape_surface(TstSuite* suite, TstItem* tstitem)
{
    ANN(suite);

    uint32_t row_count = 8;
    uint32_t col_count = 12;
    vec3 o = {0};
    vec3 u = {1, 0, 0};
    vec3 v = {0, 1, 0};
    float* heights = (float*)calloc(row_count * col_count, sizeof(float));
    uint32_t idx = 0;
    for (uint32_t i = 0; i < row_count; i++)
    {
        for (uint32_t j = 0; j < col_count; j++)
        {
            heights[idx++] = sin(i) * cos(j);
        }
    }
    DvzShape* shape = dvz_shape();
    dvz_shape_surface(shape, row_count, col_count, heights, NULL, o, u, v, 0);

    dvz_shape_print(shape);

    dvz_shape_destroy(shape);
    return 0;
}



int test_shape_transform(TstSuite* suite, TstItem* tstitem)
{
    ANN(suite);

    DvzColor color = {RED};
    const uint32_t count = 30;

    DvzShape* shape = dvz_shape();
    dvz_shape_square(shape, color);

    dvz_shape_begin(shape, 0, 0);
    dvz_shape_scale(shape, (vec3){2, .5, 1});
    dvz_shape_end(shape);
    dvz_shape_print(shape);

    dvz_shape_destroy(shape);
    return 0;
}



int test_shape_obj(TstSuite* suite, TstItem* tstitem)
{
    ANN(suite);

    char path[1024] = {0};
    snprintf(path, sizeof(path), "%s/mesh/brain.obj", DATA_DIR);

    DvzShape* shape = dvz_shape();
    dvz_shape_obj(shape, path);
    dvz_shape_print(shape);
    dvz_shape_destroy(shape);
    return 0;
}
