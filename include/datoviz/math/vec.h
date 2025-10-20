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

#include "_macros.h"
#include "types.h"



/*************************************************************************************************/
/*  Vector operations                                                                            */
/*************************************************************************************************/

DVZ_EXPORT void dvz_vec2_copy(const vec2 a, vec2 b);



DVZ_EXPORT void dvz_vec3_copy(const vec3 a, vec3 b);



DVZ_EXPORT void dvz_vec3_cast(const dvec3* a, vec3* b);



DVZ_EXPORT void dvz_dvec3_copy(const dvec3 a, dvec3 b);



DVZ_EXPORT void dvz_dvec4_copy(const dvec4 a, dvec4 b);



DVZ_EXPORT void dvz_dmat4_copy(dmat4 mat, dmat4 dest);



DVZ_EXPORT void dvz_dmat4_mat4(mat4 mat, dmat4 dest);



DVZ_EXPORT void dvz_dmat4_identity(dmat4 mat);



DVZ_EXPORT void dvz_dmat4_mul(dmat4 m1, dmat4 m2, dmat4 dest);



DVZ_EXPORT void dvz_dmat4_mulv(dmat4 m, dvec4 v, dvec4 dest);



DVZ_EXPORT void dvz_dvec4(dvec3 v3, double last, dvec4 dest);



DVZ_EXPORT void dvz_dvec3(dvec4 v4, dvec3 dest);



DVZ_EXPORT void dvz_dmat4_mulv3(dmat4 m, dvec3 v, double last, dvec3 dest);



DVZ_EXPORT void dvz_dmat4_scale_p(dmat4 m, double s);



DVZ_EXPORT void dvz_dmat4_inv(dmat4 mat, dmat4 dest);
