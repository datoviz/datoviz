/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Testing stats                                                                                */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "_assertions.h"
#include "datoviz/math/parallel.h"
#include "datoviz/math/stats.h"
#include "test_math.h"
#include "testing.h"
#include "datoviz/math/types.h"



/*************************************************************************************************/
/*  Stats tests                                                                                  */
/*************************************************************************************************/

int test_stats_parallel(TstContext* suite, const TstCase* tstitem)
{
    ANN(suite);

    double values[] = {-2.0, 4.0, 1.5};
    double mean = dvz_mean(DVZ_ARRAY_COUNT(values), values);
    AC(mean, (values[0] + values[1] + values[2]) / 3.0, EPS);

    dvec2 min_max = {0.0, 0.0};
    dvz_range(DVZ_ARRAY_COUNT(values), values, min_max);
    AC(min_max[0], -2.0, EPS);
    AC(min_max[1], 4.0, EPS);

    double negative = -5.0;
    dvz_range(1, &negative, min_max);
    AC(min_max[0], negative, EPS);
    AC(min_max[1], negative, EPS);

    dvec2 unchanged = {3.0, 7.0};
    dvz_range(0, NULL, unchanged);
    AC(unchanged[0], 3.0, EPS);
    AC(unchanged[1], 7.0, EPS);

    float float_values[] = {-2.0f, 4.0f, 1.5f};
    vec2 float_min_max = {0};
    dvz_min_max(DVZ_ARRAY_COUNT(float_values), float_values, float_min_max);
    AC(float_min_max[0], -2.0f, EPS);
    AC(float_min_max[1], 4.0f, EPS);

    float normalize_values[] = {-1.0f, 0.0f, 1.0f, 2.0f, 3.0f};
    uint8_t normalized[DVZ_ARRAY_COUNT(normalize_values)] = {0};
    dvz_normalize_bytes(
        (vec2){0.0f, 2.0f}, DVZ_ARRAY_COUNT(normalize_values), normalize_values, normalized);
    AT(normalized[0] == 0);
    AT(normalized[1] == 0);
    AT(normalized[2] == 128);
    AT(normalized[3] == 255);
    AT(normalized[4] == 255);

    float equal_bound_values[] = {2.0f, 3.0f, 4.0f};
    uint8_t equal_bound_normalized[DVZ_ARRAY_COUNT(equal_bound_values)] = {0};
    dvz_normalize_bytes(
        (vec2){3.0f, 3.0f}, DVZ_ARRAY_COUNT(equal_bound_values), equal_bound_values,
        equal_bound_normalized);
    AT(equal_bound_normalized[0] == 0);
    AT(equal_bound_normalized[1] == 0);
    AT(equal_bound_normalized[2] == 255);

    return 0;
}



int test_parallel_thread_config(TstContext* suite, const TstCase* tstitem)
{
    ANN(suite);

#if DVZ_HAS_OPENMP
    AT(dvz_threads_set(1) == DVZ_OK);
    AT(dvz_threads_default() == DVZ_OK);
#else
    AT(dvz_threads_set(1) == DVZ_ERROR);
    AT(dvz_threads_default() == DVZ_ERROR);
#endif

    return 0;
}
