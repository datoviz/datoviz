/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Unsigned 64-bit overflow-checked arithmetic (header-only)                                    */
/*************************************************************************************************/

#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "_assertions.h"



/**
 * Multiply two unsigned 64-bit integers with overflow detection.
 *
 * @param a the first operand
 * @param b the second operand
 * @param out the product output
 * @return whether the multiplication would overflow
 */
static inline bool _dvz_mul_u64_overflows(uint64_t a, uint64_t b, uint64_t* out)
{
    ANN(out);
    if (a != 0 && b > UINT64_MAX / a)
        return true;
    *out = a * b;
    return false;
}



/**
 * Add two unsigned 64-bit integers with overflow detection.
 *
 * @param a the first operand
 * @param b the second operand
 * @param out the sum output
 * @return whether the addition would overflow
 */
static inline bool _dvz_add_u64_overflows(uint64_t a, uint64_t b, uint64_t* out)
{
    ANN(out);
    if (b > UINT64_MAX - a)
        return true;
    *out = a + b;
    return false;
}



/**
 * Add three unsigned 64-bit integers with overflow detection.
 *
 * @param a the first operand
 * @param b the second operand
 * @param c the third operand
 * @param out the sum output
 * @return whether the addition would overflow
 */
static inline bool _dvz_add3_u64_overflows(uint64_t a, uint64_t b, uint64_t c, uint64_t* out)
{
    ANN(out);
    if (b > UINT64_MAX - a)
        return true;
    uint64_t ab = a + b;
    if (c > UINT64_MAX - ab)
        return true;
    *out = ab + c;
    return false;
}
