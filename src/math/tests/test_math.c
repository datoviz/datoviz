/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Testing math                                                                                 */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "_assertions.h"

#include "datoviz/math/stats.h"
#include "test_math.h"
#include "testing.h"



/*************************************************************************************************/
/*  Entry-point                                                                                  */
/*************************************************************************************************/

int test_math(TstSuite* suite)
{
    ANN(suite);

    const char* tags = "math";

    TEST_SIMPLE(test_prng_1);
    TEST_SIMPLE(test_parallel_stats);

    return 0;
}



int test_parallel_stats(TstSuite* suite, TstItem* tstitem)
{
    ANN(suite);

    double values[] = {-2.0, 4.0, 1.5};
    double mean = dvz_mean(ARRAY_COUNT(values), values);
    AC(mean, (values[0] + values[1] + values[2]) / 3.0, EPS);

    dvec2 min_max = {0.0, 0.0};
    dvz_range(ARRAY_COUNT(values), values, min_max);
    AC(min_max[0], -2.0, EPS);
    AC(min_max[1], 4.0, EPS);

    double negative = -5.0;
    dvz_range(1, &negative, min_max);
    AC(min_max[0], negative, EPS);
    AC(min_max[1], negative, EPS);

    return 0;
}
