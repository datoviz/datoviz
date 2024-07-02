/*************************************************************************************************/
/*  Common mathematical macros                                                                   */
/*************************************************************************************************/

#ifndef DVZ_MATH
#define DVZ_MATH



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <float.h>
#include <inttypes.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "_cglm.h"
#include "datoviz_macros.h"
#include "datoviz_math.h"



/*************************************************************************************************/
/*  Utils                                                                                        */
/*************************************************************************************************/

// Smallest power of 2 larger or equal than a positive integer.
inline uint64_t dvz_next_pow2(uint64_t x)
{
    uint64_t p = 1;
    while (p < x)
        p *= 2;
    return p;
}



/**
 * Compute the mean of an array of double values.
 *
 * @param n the number of values
 * @param values an array of double numbers
 * @returns the mean
 */
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



/**
 * Compute the min and max of an array of float values.
 *
 * @param n the number of values
 * @param values an array of float numbers
 * @param vec2 the min and max
 * @returns the mean
 */
inline void dvz_min_max(uint32_t n, const float* values, vec2 out_min_max)
{
    ASSERT(n > 0);
    ASSERT(values != NULL);
    float m = 0, M = 0;
    for (uint32_t i = 0; i < n; i++)
    {
        m = MIN(m, values[i]);
        M = MAX(M, values[i]);
    }
    ASSERT(m <= M);
    out_min_max[0] = m;
    out_min_max[1] = M;
}



/**
 * Normalize the array.
 *
 * @param count the number of values
 * @param values an array of float numbers
 * @returns the normalized array
 */
inline uint8_t* dvz_normalize_bytes(uint32_t count, float* values)
{
    ASSERT(count > 0);
    ANN(values);

    vec2 min_max = {0};
    dvz_min_max(count, values, min_max);
    float m = min_max[0];
    float M = min_max[1];
    if (m == M)
        M = m + 1;
    ASSERT(m < M);
    float d = 1. / (M - m);

    uint8_t* out = (uint8_t*)malloc(count * sizeof(uint8_t));

    for (uint32_t i = 0; i < count; i++)
    {
        out[i] = round((values[i] - m) * d * 255);
    }

    return out;
}



/**
 * Compute the range of an array of double values.
 *
 * @param n the number of values
 * @param values an array of double numbers
 * @param[out] the min and max values
 */
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

/**
 * Return a random integer number between 0 and 255.
 *
 * @returns random number
 */
inline uint8_t dvz_rand_byte(void) { return (uint8_t)(rand() % 256); }



/**
 * Return a random integer number.
 *
 * @returns random number
 */
inline int dvz_rand_int(void) { return rand(); }



/**
 * Return a random floating-point number between 0 and 1.
 *
 * @returns random number
 */
inline float dvz_rand_float(void) { return (float)rand() / (float)(RAND_MAX); }



/**
 * Return a random floating-point number between 0 and 1.
 *
 * @returns random number
 */
inline double dvz_rand_double(void) { return (double)rand() / (double)(RAND_MAX); }



/**
 * Return a random normal floating-point number.
 *
 * @returns random number
 */
inline double dvz_rand_normal(void)
{
    return sqrt(-2.0 * log(dvz_rand_double())) * cos(2 * M_PI * dvz_rand_double());
}



#endif
