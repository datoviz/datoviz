/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Math                                                                                         */
/*************************************************************************************************/

#pragma once



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

#if HAS_OPENMP
#include <omp.h>
#endif

#include "common/enums.h"
#include "common/macros.h"
#include "math/mock.h"
#include "math/rand.h"
#include "math/types.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#define M_2PI 6.28318530717958647692
// #define M_PI_2 1.57079632679489650726

#define M_INV_255 0.00392156862745098

#define EPSILON 1e-10

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
static uint32_t _ZERO_OFFSET[3] = {0, 0, 0};
#pragma GCC diagnostic pop
#define DVZ_ZERO_OFFSET _ZERO_OFFSET



/*************************************************************************************************/
/*  Macros                                                                                       */
/*************************************************************************************************/

#define MIN(a, b)     (((a) < (b)) ? (a) : (b))
#define MAX(a, b)     (((a) > (b)) ? (a) : (b))
#define CLIP(x, a, b) MAX(MIN((x), (b)), (a))

// #define TO_BYTE(x)   (uint8_t)round(CLIP((x), 0, 1) * 255)
// #define FROM_BYTE(x) ((x) / 255.0)

#define _DMAT4_IDENTITY_INIT                                                                      \
    {                                                                                             \
        {1.0, 0.0, 0.0, 0.0}, {0.0, 1.0, 0.0, 0.0}, {0.0, 0.0, 1.0, 0.0}, { 0.0, 0.0, 0.0, 1.0 }  \
    }



// TODO: move elsewhere
static inline uint64_t dvz_next_pow2(uint64_t x)
{
    uint64_t p = 1;
    while (p < x)
        p *= 2;
    return p;
}
