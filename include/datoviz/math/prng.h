/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Random 64-bit integer                                                                        */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "datoviz/common/macros.h"
#include <stdint.h>



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzPrng DvzPrng;



EXTERN_C_ON

/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Create a pseudorandom number generator seeded from the platform random device.
 *
 * @return a new owned generator; destroy it with `dvz_prng_destroy()`
 */
DVZ_EXPORT DvzPrng* dvz_prng(void);



/**
 * Generate the next pseudorandom 64-bit value.
 *
 * @param prng generator state to advance; must not be NULL
 * @return pseudorandom value in the inclusive range [0, UINT64_MAX]
 */
DVZ_EXPORT uint64_t dvz_prng_uuid(DvzPrng* prng);



/**
 * Destroy a pseudorandom number generator.
 *
 * @param prng owned generator to destroy; must not be NULL
 */
DVZ_EXPORT void dvz_prng_destroy(DvzPrng* prng);



EXTERN_C_OFF
