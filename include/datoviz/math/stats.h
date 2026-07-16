/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Stats                                                                                        */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "datoviz/common/macros.h"
#include "datoviz/math/types.h"
#include "types.h"
#include <float.h>
#include <inttypes.h>
#include <stdbool.h>


EXTERN_C_ON

/*************************************************************************************************/
/*  Stats                                                                                   */
/*************************************************************************************************/

/**
 * Compute the mean of an array of double values.
 *
 * @param n number of values; must be positive
 * @param values input array containing @p n values; must not be NULL
 * @return arithmetic mean
 */
DVZ_EXPORT double dvz_mean(uint32_t n, const double* values);



/**
 * Compute the min and max of an array of float values.
 *
 * @param n number of values; must be positive
 * @param values input array containing @p n values; must not be NULL
 * @param[out] out_min_max destination receiving `{minimum, maximum}`
 */
DVZ_EXPORT void dvz_min_max(uint32_t n, const float* values, vec2 out_min_max);



/**
 * Map floating-point values linearly to unsigned bytes.
 *
 * Values at or below the selected minimum map to 0, and values at or above the maximum map to
 * 255. When both bounds are equal, the effective maximum is `minimum + 1`.
 *
 * @param min_max input bounds `{minimum, maximum}`; minimum must not exceed maximum
 * @param count number of values; must be positive
 * @param values input array containing @p count values; must not be NULL
 * @param[out] out destination array receiving @p count normalized bytes; must not be NULL
 */
DVZ_EXPORT void dvz_normalize_bytes(vec2 min_max, uint32_t count, float* values, uint8_t* out);



/**
 * Compute the range of an array of double values.
 *
 * If @p n is zero, this function returns without modifying @p min_max.
 *
 * @param n number of input values
 * @param values input array containing @p n values; must not be NULL when @p n is positive
 * @param[out] min_max destination receiving `{minimum, maximum}` when @p n is positive
 */
DVZ_EXPORT void dvz_range(uint32_t n, const double* values, dvec2 min_max);



EXTERN_C_OFF
