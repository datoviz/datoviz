/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Mock functions                                                                               */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "datoviz/color/enums.h"
#include "datoviz/color/types.h"
#include "datoviz/common/macros.h"
#include "datoviz/math/types.h"
#include <float.h>
#include <inttypes.h>
#include <stdbool.h>


EXTERN_C_ON

/*************************************************************************************************/
/*  Mock random data                                                                             */
/*************************************************************************************************/

/**
 * Generate a set of random 2D positions.
 *
 * @param count the number of positions to generate
 * @param std the standard deviation
 * @returns the positions
 */
DVZ_EXPORT vec3* dvz_mock_pos_2D(uint32_t count, float std);



/**
 * Generate points on a circle.
 *
 * @param count the number of positions to generate
 * @param radius the radius of the circle
 * @returns the positions
 */
DVZ_EXPORT vec3* dvz_mock_circle(uint32_t count, float radius);



/**
 * Generate points on a band.
 *
 * @param count the number of positions to generate
 * @param size the size of the band
 * @returns the positions
 */
DVZ_EXPORT vec3* dvz_mock_band(uint32_t count, vec2 size);



/**
 * Generate a set of random 3D positions.
 *
 * @param count the number of positions to generate
 * @param std the standard deviation
 * @returns the positions
 */
DVZ_EXPORT vec3* dvz_mock_pos_3D(uint32_t count, float std);



/**
 * Generate identical 3D positions.
 *
 * @param count the number of positions to generate
 * @param fixed the position
 * @returns the repeated positions
 */
DVZ_EXPORT vec3* dvz_mock_fixed(uint32_t count, vec3 fixed);



/**
 * Generate 3D positions on a line.
 *
 * @param count the number of positions to generate
 * @param p0 initial position
 * @param p1 terminal position
 * @returns the positions
 */
DVZ_EXPORT vec3* dvz_mock_line(uint32_t count, vec3 p0, vec3 p1);



/**
 * Generate a set of uniformly random scalar values.
 *
 * @param count the number of values to generate
 * @param vmin the minimum value of the interval
 * @param vmax the maximum value of the interval
 * @returns the values
 */
DVZ_EXPORT float* dvz_mock_uniform(uint32_t count, float vmin, float vmax);



/**
 * Generate an array with the same value.
 *
 * @param count the number of scalars to generate
 * @param value the value
 * @returns the values
 */
DVZ_EXPORT float* dvz_mock_full(uint32_t count, float value);



/**
 * Generate an array of consecutive positive numbers.
 *
 * @param count the number of consecutive integers to generate
 * @param initial the initial value
 * @returns the values
 */
DVZ_EXPORT uint32_t* dvz_mock_range(uint32_t count, uint32_t initial);



/**
 * Generate an array ranging from an initial value to a final value.
 *
 * @param count the number of scalars to generate
 * @param initial the initial value
 * @param final the final value
 * @returns the values
 */
DVZ_EXPORT float* dvz_mock_linspace(uint32_t count, float initial, float final);



/**
 * Generate a set of random colors.
 *
 * @param count the number of colors to generate
 * @param alpha the alpha value
 * @returns random colors
 */
DVZ_EXPORT DvzColor* dvz_mock_color(uint32_t count, DvzAlpha alpha);



/**
 * Repeat a color in an array.
 *
 * @param count the number of colors to generate
 * @param mono the color to repeat
 * @returns colors
 */
DVZ_EXPORT DvzColor* dvz_mock_monochrome(uint32_t count, DvzColor mono);



/**
 * Generate a set of colormap colors.
 *
 * @param count the number of colors to generate
 * @param cmap the colormap
 * @param alpha the alpha value
 * @returns colors
 */
DVZ_EXPORT DvzColor* dvz_mock_cmap(uint32_t count, DvzColormap cmap, DvzAlpha alpha);



EXTERN_C_OFF
