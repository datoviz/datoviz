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
#include <stddef.h>

#include "_assertions.h"
#include "datoviz/math/parallel.h"
#include "datoviz/math/types.h"



/*************************************************************************************************/
/*  OpenMP                                                                                       */
/*************************************************************************************************/

static int NUM_THREADS;

int dvz_num_procs(void)
{
#if HAS_OPENMP
    return omp_get_num_procs();
#else
    return 0;
#endif
}



void dvz_threads_set(int num_threads)
{
#if HAS_OPENMP
    int num_procs = dvz_num_procs();
    if (num_threads <= 0)
    {
        num_threads += num_procs;
    }
    num_threads = MIN(num_threads, num_procs);
    ASSERT(1 <= num_threads);
    ASSERT(num_threads <= num_procs);
    log_info("setting the number of OpenMP threads to %d/%d", num_threads, num_procs);
    NUM_THREADS = num_threads;
    omp_set_num_threads(num_threads);
#endif
}



int dvz_threads_get(void)
{
#if HAS_OPENMP
    return NUM_THREADS;
#else
    return 0;
#endif
}



void dvz_threads_default(void)
{
#if HAS_OPENMP
    // Set number of threads from DVZ_NUM_THREADS env variable.
    char* env = getenv("DVZ_NUM_THREADS");
    if (env == NULL)
    {
        int n = dvz_num_procs();
        n = MAX(1, n / 2);
        ASSERT(1 <= n);
        dvz_threads_set(n);
    }
    else
    {
        int num_threads = getenvint("DVZ_NUM_THREADS");
        dvz_threads_set(num_threads);
    }
#endif
}



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
    return mean / (double)n;
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
    ASSERT(n > 0);
    ASSERT(values != NULL);
    ASSERT(min_max != NULL);
    min_max[0] = DBL_MAX;
    min_max[1] = -DBL_MAX;
    for (uint32_t i = 0; i < n; i++)
    {
        double val = values[i];
        if (val < min_max[0])
            min_max[0] = val;
        if (val > min_max[1])
            min_max[1] = val;
    }
}
