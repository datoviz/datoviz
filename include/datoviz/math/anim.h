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

#include <float.h>
#include <inttypes.h>
#include <stdbool.h>

#include "datoviz/common/macros.h"
#include "datoviz/math/types.h"
#include "types.h"



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/

// Easing.
typedef enum
{
    DVZ_EASING_NONE,
    DVZ_EASING_IN_SINE,
    DVZ_EASING_OUT_SINE,
    DVZ_EASING_IN_OUT_SINE,
    DVZ_EASING_IN_QUAD,
    DVZ_EASING_OUT_QUAD,
    DVZ_EASING_IN_OUT_QUAD,
    DVZ_EASING_IN_CUBIC,
    DVZ_EASING_OUT_CUBIC,
    DVZ_EASING_IN_OUT_CUBIC,
    DVZ_EASING_IN_QUART,
    DVZ_EASING_OUT_QUART,
    DVZ_EASING_IN_OUT_QUART,
    DVZ_EASING_IN_QUINT,
    DVZ_EASING_OUT_QUINT,
    DVZ_EASING_IN_OUT_QUINT,
    DVZ_EASING_IN_EXPO,
    DVZ_EASING_OUT_EXPO,
    DVZ_EASING_IN_OUT_EXPO,
    DVZ_EASING_IN_CIRC,
    DVZ_EASING_OUT_CIRC,
    DVZ_EASING_IN_OUT_CIRC,
    DVZ_EASING_IN_BACK,
    DVZ_EASING_OUT_BACK,
    DVZ_EASING_IN_OUT_BACK,
    DVZ_EASING_IN_ELASTIC,
    DVZ_EASING_OUT_ELASTIC,
    DVZ_EASING_IN_OUT_ELASTIC,
    DVZ_EASING_IN_BOUNCE,
    DVZ_EASING_OUT_BOUNCE,
    DVZ_EASING_IN_OUT_BOUNCE,
    DVZ_EASING_COUNT,
} DvzEasing;



EXTERN_C_ON

/*************************************************************************************************/
/*  Animations                                                                                   */
/*************************************************************************************************/

/**
 * Normalize a value in an interval.
 *
 * @param t0 the interval start
 * @param t1 the interval end
 * @param t the value within the interval
 * @returns the normalized value between 0 and 1
 */
DVZ_EXPORT double dvz_resample(double t0, double t1, double t);



/**
 * Apply an easing function to a normalized value.
 *
 * @param easing the easing mode
 * @param t the normalized value
 * @returns the eased value
 */
DVZ_EXPORT double dvz_easing(DvzEasing easing, double t);



/**
 * Generate a 2D circular motion.
 *
 * @param center the circle center
 * @param radius the circle radius
 * @param angle the initial angle
 * @param t the normalized value
 * @param[out] out the 2D position
 */
DVZ_EXPORT void dvz_circular_2D(vec2 center, float radius, float angle, float t, vec2 out);



/**
 * Generate a 3D circular motion.
 *
 * @param pos_init the initial position
 * @param center the center position
 * @param axis the axis around which to rotate
 * @param t the normalized value (1 = full circle)
 * @param[out] out the 3D position
 */
DVZ_EXPORT void dvz_circular_3D(vec3 pos_init, vec3 center, vec3 axis, float t, vec3 out);



/**
 * Make a linear interpolation between two scalar value.
 *
 * @param p0 the first value
 * @param p1 the second value
 * @param t the normalized value
 * @returns the interpolated value
 */
DVZ_EXPORT float dvz_interpolate(float p0, float p1, float t);



/**
 * Make a linear interpolation between two 2D points.
 *
 * @param p0 the first point
 * @param p1 the second point
 * @param t the normalized value
 * @param[out] out the interpolated point
 */
DVZ_EXPORT void dvz_interpolate_2D(vec2 p0, vec2 p1, float t, vec2 out);



/**
 * Make a linear interpolation between two 3D points.
 *
 * @param p0 the first point
 * @param p1 the second point
 * @param t the normalized value
 * @param[out] out the interpolated point
 */
DVZ_EXPORT void dvz_interpolate_3D(vec3 p0, vec3 p1, float t, vec3 out);



EXTERN_C_OFF
