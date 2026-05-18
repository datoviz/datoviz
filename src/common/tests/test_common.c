/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Testing common                                                                               */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "_assertions.h"

#include "test_common.h"
#include "testing.h"



/*************************************************************************************************/
/*  Entry-point                                                                                  */
/*************************************************************************************************/

int test_common(TstSuite* suite)
{
    ANN(suite);

    const char* tags = "common";

    TST_MODULE(suite, "common");
    TST_GROUP("obj");
    TST_CASE(test_obj_1);

    TST_GROUP("alloc");
    TST_CASE(test_alloc_basic);
    TST_CASE(test_alloc_aligned);

    return 0;
}
