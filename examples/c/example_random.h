/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Example random helpers                                                                       */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdint.h>



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct ExampleRandom
{
    uint32_t state;
} ExampleRandom;



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

ExampleRandom example_random(uint32_t seed);

uint32_t example_random_u32(ExampleRandom* rng);

float example_random_f32(ExampleRandom* rng);

float example_random_range_f32(ExampleRandom* rng, float min, float max);

uint8_t example_random_u8(ExampleRandom* rng);
