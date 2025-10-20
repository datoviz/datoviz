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
#include <stdlib.h>

// #include "_cglm.h"
#include "log.h"
// #include "datoviz.h" // for dvz_colormap_scale() used in mock
#include "datoviz/common/assert.h"
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
    mean /= n;
    ASSERT(mean >= 0);
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
    min_max[0] = FLT_MAX;
    min_max[1] = FLT_MIN;
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



/*************************************************************************************************/
/*  Random number generation                                                                     */
/*************************************************************************************************/

inline uint8_t dvz_rand_byte(void) { return (uint8_t)(rand() % 256); }



inline int dvz_rand_int(void) { return rand(); }



inline float dvz_rand_float(void) { return (float)rand() / (float)(RAND_MAX); }



inline double dvz_rand_double(void) { return (double)rand() / (double)(RAND_MAX); }



inline double dvz_rand_normal(void)
{
    return sqrt(-2.0 * log(dvz_rand_double())) * cos(2 * M_PI * dvz_rand_double());
}



/*************************************************************************************************/
/*  Mock random data                                                                             */
/*************************************************************************************************/

// vec3* dvz_mock_pos_2D(uint32_t count, float std)
// {
//     ASSERT(count > 0);
//     vec3* pos = (vec3*)calloc(count, sizeof(vec3));
//     for (uint32_t i = 0; i < count; i++)
//     {
//         pos[i][0] = std * dvz_rand_normal();
//         pos[i][1] = std * dvz_rand_normal();
//     }
//     return pos;
// }



// vec3* dvz_mock_circle(uint32_t count, float radius)
// {
//     ASSERT(count > 1);
//     vec3* pos = (vec3*)calloc(count, sizeof(vec3));
//     // bool closed = (flags & DVZ_MOCK_FLAGS_CLOSED) > 0;
//     uint32_t n = count; // closed ? count - 1 : count;
//     for (uint32_t i = 0; i < count; i++)
//     {
//         pos[i][0] = radius * cos(M_2PI * i * 1.0 / n);
//         pos[i][1] = radius * sin(M_2PI * i * 1.0 / n);
//     }
//     return pos;
// }



// vec3* dvz_mock_band(uint32_t count, vec2 size)
// {
//     ASSERT(count > 0);
//     float a = size[0] * .5;
//     float b = size[1] * .5;
//     vec3* pos = (vec3*)calloc(count, sizeof(vec3));
//     for (uint32_t i = 0; i < count; i++)
//     {
//         pos[i][0] = -a + 2 * a * (i / 2) * 1. / (count / 2 - 1);
//         pos[i][1] = -b + 2 * b * (i % 2);
//     }
//     return pos;
// }



// vec3* dvz_mock_pos_3D(uint32_t count, float std)
// {
//     ASSERT(count > 0);
//     vec3* pos = (vec3*)calloc(count, sizeof(vec3));
//     for (uint32_t i = 0; i < count; i++)
//     {
//         pos[i][0] = std * dvz_rand_normal();
//         pos[i][1] = std * dvz_rand_normal();
//         pos[i][2] = std * dvz_rand_normal();
//     }
//     return pos;
// }



// vec3* dvz_mock_fixed(uint32_t count, vec3 fixed)
// {
//     ASSERT(count > 0);
//     vec3* pos = (vec3*)calloc(count, sizeof(vec3));

//     for (uint32_t i = 0; i < count; i++)
//     {
//         pos[i][0] = fixed[0];
//         pos[i][1] = fixed[1];
//         pos[i][2] = fixed[2];
//     }
//     return pos;
// }



// vec3* dvz_mock_line(uint32_t count, vec3 p0, vec3 p1)
// {
//     ASSERT(count > 1);
//     vec3* pos = (vec3*)calloc(count, sizeof(vec3));

//     vec3 u = {0};
//     u[0] = (p1[0] - p0[0]) * 1. / (count - 1);
//     u[1] = (p1[1] - p0[1]) * 1. / (count - 1);
//     u[2] = (p1[2] - p0[2]) * 1. / (count - 1);

//     for (uint32_t i = 0; i < count; i++)
//     {
//         pos[i][0] = p0[0] + i * u[0];
//         pos[i][1] = p0[1] + i * u[1];
//         pos[i][2] = p0[2] + i * u[2];
//     }
//     return pos;
// }



// float* dvz_mock_uniform(uint32_t count, float vmin, float vmax)
// {
//     ASSERT(count > 0);
//     ASSERT(vmin <= vmax);
//     float* size = (float*)calloc(count, sizeof(float));
//     float a = vmax - vmin;
//     for (uint32_t i = 0; i < count; i++)
//     {
//         size[i] = vmin + a * dvz_rand_float();
//     }
//     return size;
// }



// float* dvz_mock_full(uint32_t count, float value)
// {
//     ASSERT(count > 0);
//     float* values = (float*)calloc(count, sizeof(float));
//     for (uint32_t i = 0; i < count; i++)
//     {
//         values[i] = value;
//     }
//     return values;
// }



// uint32_t* dvz_mock_range(uint32_t count, uint32_t initial)
// {
//     ASSERT(count > 1);
//     uint32_t* values = (uint32_t*)calloc(count, sizeof(uint32_t));
//     for (uint32_t i = 0; i < count; i++)
//     {
//         values[i] = initial + i;
//     }
//     return values;
// }



// float* dvz_mock_linspace(uint32_t count, float initial, float final)
// {
//     ASSERT(count > 1);
//     float* values = (float*)calloc(count, sizeof(float));
//     for (uint32_t i = 0; i < count; i++)
//     {
//         values[i] = initial + (final - initial) * i * 1.0 / (count - 1);
//     }
//     return values;
// }



/*************************************************************************************************/
/*  Mock colors                                                                                  */
/*************************************************************************************************/

// DvzColor* dvz_mock_color(uint32_t count, DvzAlpha alpha)
// {
//     ASSERT(count > 0);
//     DvzColor* color = (DvzColor*)calloc(count, sizeof(DvzColor));
//     const uint8_t k = 64;
//     for (uint32_t i = 0; i < count; i++)
//     {
//         // dvz_colormap(DVZ_CMAP_HSV, i % 256, color[i]);
//         color[i][0] = k + (dvz_rand_byte() % (256 - k));
//         color[i][1] = k + (dvz_rand_byte() % (256 - k));
//         color[i][2] = k + (dvz_rand_byte() % (256 - k));
//         color[i][3] = alpha;
//     }
//     return color;
// }



// DvzColor* dvz_mock_monochrome(uint32_t count, DvzColor mono)
// {
//     ASSERT(count > 0);
//     DvzColor* color = (DvzColor*)calloc(count, sizeof(DvzColor));
//     for (uint32_t i = 0; i < count; i++)
//     {
//         color[i][0] = mono[0];
//         color[i][1] = mono[1];
//         color[i][2] = mono[2];
//         color[i][3] = mono[3];
//     }
//     return color;
// }



// DvzColor* dvz_mock_cmap(uint32_t count, DvzColormap cmap, DvzAlpha alpha)
// {
//     ASSERT(count > 0);
//     DvzColor* color = (DvzColor*)calloc(count, sizeof(DvzColor));
//     for (uint32_t i = 0; i < count; i++)
//     {
//         dvz_colormap_scale(cmap, i, 0, count, color[i]);
//         color[i][3] = alpha;
//     }
//     return color;
// }
