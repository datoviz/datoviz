/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Math arithmetic utils                                                                        */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <float.h>
#include <inttypes.h>
#include <stdbool.h>



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Return the smallest power of two greater than or equal to an integer.
 *
 * @param x input integer
 * @return the next power of two, with zero and one both mapping to one
 *
 * @note Values above `2^63` overflow the `uint64_t` calculation and are unsupported.
 */
static inline uint64_t dvz_next_pow2(uint64_t x)
{
    uint64_t p = 1;
    while (p < x)
        p *= 2;
    return p;
}
