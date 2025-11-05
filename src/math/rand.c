/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Math random                                                                                  */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <inttypes.h>
#include <math.h>
#include <stdlib.h>

#include "datoviz/math/rand.h"



/*************************************************************************************************/
/*  Random number generation                                                                     */
/*************************************************************************************************/

inline uint8_t dvz_rand_byte(void) { return (uint8_t)(rand() % 256); }



inline int dvz_rand_int(void) { return rand(); }



inline float dvz_rand_float(void) { return (float)rand() / (float)(RAND_MAX); }



inline double dvz_rand_double(void) { return (double)rand() / (double)(RAND_MAX); }



inline double dvz_rand_normal(void)
{
    return sqrt(-2.0 * log(dvz_rand_double())) * cos(2 * M_PI * dvz_rand_double());
}
