/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Animation functions                                                                          */
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
 * Apply an easing function to a normalized value.
 *
 * @param easing easing curve to apply
 * @param t normalized input, normally in the interval [0, 1]
 * @return eased value; some curves intentionally overshoot [0, 1]
 */
DVZ_EXPORT double dvz_easing(DvzEasing easing, double t);


EXTERN_C_OFF
