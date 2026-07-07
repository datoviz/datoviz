/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Example random helpers                                                                       */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "example_random.h"

#include "_assertions.h"



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Return a deterministic example RNG state.
 *
 * @param seed initial seed; zero is remapped to a non-zero xorshift state
 * @return initialized RNG
 */
ExampleRandom example_random(uint32_t seed)
{
    return (ExampleRandom){.state = seed != 0 ? seed : 0xA341316Cu};
}



/**
 * Return the next pseudo-random 32-bit integer.
 *
 * @param rng RNG state
 * @return random integer
 */
uint32_t example_random_u32(ExampleRandom* rng)
{
    ANN(rng);

    uint32_t value = rng->state;
    value ^= value << 13;
    value ^= value >> 17;
    value ^= value << 5;
    rng->state = value;
    return value;
}



/**
 * Return a pseudo-random float in [0, 1).
 *
 * @param rng RNG state
 * @return random scalar
 */
float example_random_f32(ExampleRandom* rng)
{
    return (float)(example_random_u32(rng) >> 8) / (float)0x01000000u;
}



/**
 * Return a pseudo-random float in [min, max).
 *
 * @param rng RNG state
 * @param min lower bound
 * @param max upper bound
 * @return random scalar
 */
float example_random_range_f32(ExampleRandom* rng, float min, float max)
{
    return min + (max - min) * example_random_f32(rng);
}



/**
 * Return a pseudo-random byte.
 *
 * @param rng RNG state
 * @return random byte
 */
uint8_t example_random_u8(ExampleRandom* rng)
{
    return (uint8_t)(example_random_u32(rng) >> 24);
}
