/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Common random macros                                                                         */
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

#include "datoviz/common/macros.h"



EXTERN_C_ON

/*************************************************************************************************/
/*  Random number generation                                                                     */
/*************************************************************************************************/

/**
 * Return a random integer number between 0 and 255.
 *
 * @returns random number
 */
DVZ_EXPORT uint8_t dvz_rand_byte(void);



/**
 * Return a random integer number.
 *
 * @returns random number
 */
DVZ_EXPORT int dvz_rand_int(void);



/**
 * Return a random floating-point number between 0 and 1.
 *
 * @returns random number
 */
DVZ_EXPORT float dvz_rand_float(void);



/**
 * Return a random floating-point number between 0 and 1.
 *
 * @returns random number
 */
DVZ_EXPORT double dvz_rand_double(void);



/**
 * Return a random normal floating-point number.
 *
 * @returns random number
 */
DVZ_EXPORT double dvz_rand_normal(void);



EXTERN_C_OFF
