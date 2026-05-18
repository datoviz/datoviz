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

    TST_MODULE(suite, "math");
    TST_GROUP("prng");
    TST_CASE(test_prng_1);

    TST_GROUP("box");
    TST_CASE(test_box_1);
    TST_CASE(test_box_2);
    TST_CASE(test_box_3);
    TST_CASE(test_box_4);
    TST_CASE(test_box_5);
    TST_CASE(test_box_6);

    TST_GROUP("array");
    TST_CASE(test_array_1);
    TST_CASE(test_array_2);
    TST_CASE(test_array_3);
    TST_CASE(test_array_4);
    TST_CASE(test_array_5);
    TST_CASE(test_array_6);
    TST_CASE(test_array_7);
    TST_CASE(test_array_cast);
    TST_CASE(test_array_mvp);
    TST_CASE(test_array_3D);

    TST_GROUP("stats");
    TST_CASE(test_stats_parallel);

    TST_GROUP("anim");
    TST_CASE(test_anim_1);

    return 0;
}
