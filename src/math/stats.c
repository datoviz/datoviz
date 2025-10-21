/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Common mathematical macros                                                                   */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <float.h>
#include <inttypes.h>
#include <math.h>
#include <stdbool.h>

#include "_assertions.h"
#include "datoviz/math/stats.h"
#include "datoviz/math/types.h"



/*************************************************************************************************/
/*  Utils                                                                                        */
/*************************************************************************************************/

inline double dvz_mean(uint32_t n, double* values)
{
    ASSERT(n > 0);
    ASSERT(values != NULL);
    double mean = 0;
    for (uint32_t i = 0; i < n; i++)
        mean += values[i];
    mean /= n;
    return mean;
}



inline void dvz_min_max(uint32_t n, const float* values, vec2 out_min_max)
{
    ASSERT(n > 0);
    ASSERT(values != NULL);
    float m = +INFINITY, M = -INFINITY;
    for (uint32_t i = 0; i < n; i++)
    {
        m = MIN(m, values[i]);
        M = MAX(M, values[i]);
    }
    ASSERT(m <= M);
    out_min_max[0] = m;
    out_min_max[1] = M;
}



inline void dvz_normalize_bytes(vec2 min_max, uint32_t count, float* values, uint8_t* out)
{
    ASSERT(count > 0);
    ANN(values);
    ANN(out);

    float m = min_max[0];
    float M = min_max[1];
    if (m == M)
        M = m + 1;
    ASSERT(m < M);
    float d = 1. / (M - m);

    float x = 0;
    for (uint32_t i = 0; i < count; i++)
    {
        x = (values[i] - m) * d;
        x = CLIP(x, 0, 1);
        out[i] = round(x * 255);
    }
}



inline void dvz_range(uint32_t n, double* values, dvec2 min_max)
{
    if (n == 0)
        return;
    ASSERT(n > 0);
    ASSERT(values != NULL);
    min_max[0] = DBL_MAX;
    min_max[1] = -DBL_MAX;
    double val = 0;
    for (uint32_t i = 0; i < n; i++)
    {
        val = values[i];
        if (val < min_max[0])
            min_max[0] = val;

        if (val > min_max[1])
            min_max[1] = val;
    }
}
