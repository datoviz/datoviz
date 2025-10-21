/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Testing math                                                                                 */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "_assertions.h"

#include "test_math.h"
#include "testing.h"



/*************************************************************************************************/
/*  Entry-point                                                                                  */
/*************************************************************************************************/

int test_math(TstSuite* suite)
{
    ANN(suite);

    const char* tags = "math";

    TEST_SIMPLE(test_prng_1);

    TEST_SIMPLE(test_box_1);
    TEST_SIMPLE(test_box_2);
    TEST_SIMPLE(test_box_3);
    TEST_SIMPLE(test_box_4);
    TEST_SIMPLE(test_box_5);
    TEST_SIMPLE(test_box_6);

    TEST_SIMPLE(test_stats_parallel);
    TEST_SIMPLE(test_anim_1);

    return 0;
}
