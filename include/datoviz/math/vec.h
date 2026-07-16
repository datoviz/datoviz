/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Math vector functions                                                                        */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <string.h>

#include "datoviz/common/macros.h"
#include "types.h"



EXTERN_C_ON



/*************************************************************************************************/
/*  Vector operations                                                                            */
/*************************************************************************************************/

/**
 * Copy a two-component single-precision vector.
 *
 * @param a source vector
 * @param[out] b destination vector
 */
DVZ_EXPORT void dvz_vec2_copy(const vec2 a, vec2 b);



/**
 * Copy a three-component single-precision vector.
 *
 * @param a source vector
 * @param[out] b destination vector
 */
DVZ_EXPORT void dvz_vec3_copy(const vec3 a, vec3 b);



/**
 * Cast a three-component double-precision vector to single precision.
 *
 * @param a source vector; must not be NULL
 * @param[out] b destination vector; must not be NULL
 */
DVZ_EXPORT void dvz_vec3_cast(const dvec3* a, vec3* b);



/**
 * Copy a three-component double-precision vector.
 *
 * @param a source vector
 * @param[out] b destination vector
 */
DVZ_EXPORT void dvz_dvec3_copy(const dvec3 a, dvec3 b);



/**
 * Copy a four-component double-precision vector.
 *
 * @param a source vector
 * @param[out] b destination vector
 */
DVZ_EXPORT void dvz_dvec4_copy(const dvec4 a, dvec4 b);



/**
 * Copy a double-precision 4x4 matrix.
 *
 * @param mat source matrix
 * @param[out] dest destination matrix
 */
DVZ_EXPORT void dvz_dmat4_copy(dmat4 mat, dmat4 dest);



/**
 * Cast a single-precision 4x4 matrix to double precision.
 *
 * @param mat source matrix
 * @param[out] dest destination matrix
 */
DVZ_EXPORT void dvz_dmat4_mat4(mat4 mat, dmat4 dest);



/**
 * Set a double-precision 4x4 matrix to the identity matrix.
 *
 * @param[out] mat matrix to initialize
 */
DVZ_EXPORT void dvz_dmat4_identity(dmat4 mat);



/**
 * Multiply two double-precision 4x4 matrices.
 *
 * @param m1 left operand
 * @param m2 right operand
 * @param[out] dest product matrix; may alias either operand
 */
DVZ_EXPORT void dvz_dmat4_mul(dmat4 m1, dmat4 m2, dmat4 dest);



/**
 * Multiply a double-precision 4x4 matrix by a four-component vector.
 *
 * @param m matrix operand
 * @param v vector operand
 * @param[out] dest product vector; may alias @p v
 */
DVZ_EXPORT void dvz_dmat4_mulv(dmat4 m, dvec4 v, dvec4 dest);



/**
 * Extend a three-component vector with a fourth component.
 *
 * @param v3 first three components
 * @param last fourth component, commonly 1 for a position or 0 for a direction
 * @param[out] dest resulting four-component vector
 */
DVZ_EXPORT void dvz_dvec4(dvec3 v3, double last, dvec4 dest);



/**
 * Copy the first three components of a four-component vector.
 *
 * @param v4 source vector
 * @param[out] dest resulting three-component vector
 */
DVZ_EXPORT void dvz_dvec3(dvec4 v4, dvec3 dest);



/**
 * Transform a three-component vector by a double-precision 4x4 matrix.
 *
 * @param m transformation matrix
 * @param v source vector
 * @param last homogeneous fourth component, commonly 1 for a position or 0 for a direction
 * @param[out] dest first three components of the transformed vector; may alias @p v
 */
DVZ_EXPORT void dvz_dmat4_mulv3(dmat4 m, dvec3 v, double last, dvec3 dest);



/**
 * Multiply every component of a double-precision 4x4 matrix in place.
 *
 * @param[in,out] m matrix to scale
 * @param s scalar multiplier
 */
DVZ_EXPORT void dvz_dmat4_scale_p(dmat4 m, double s);



/**
 * Invert a double-precision 4x4 matrix.
 *
 * @param mat source matrix; must be invertible
 * @param[out] dest inverse matrix; may alias @p mat
 */
DVZ_EXPORT void dvz_dmat4_inv(dmat4 mat, dmat4 dest);



EXTERN_C_OFF
