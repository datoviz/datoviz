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

#include "_macros.h"
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
 * @param n the number of values
 * @param values an array of double numbers
 * @returns the mean
 */
DVZ_EXPORT double dvz_mean(uint32_t n, double* values);



/**
 * Compute the min and max of an array of float values.
 *
 * @param n the number of values
 * @param values an array of float numbers
 * @param out_min_max the min and max
 * @returns the mean
 */
DVZ_EXPORT void dvz_min_max(uint32_t n, const float* values, vec2 out_min_max);



/**
 * Normalize the array.
 *
 * @param min_max the minimum and maximum values, mapped to 0 and 255, the result will be clipped
 * @param count the number of values
 * @param values an array of float numbers
 * @param out the out uint8 array
 */
DVZ_EXPORT void dvz_normalize_bytes(vec2 min_max, uint32_t count, float* values, uint8_t* out);



/**
 * Compute the range of an array of double values.
 *
 * @param n the number of values
 * @param values an array of double numbers
 * @param[out] min_max the min and max values
 */
DVZ_EXPORT void dvz_range(uint32_t n, double* values, dvec2 min_max);



EXTERN_C_OFF
