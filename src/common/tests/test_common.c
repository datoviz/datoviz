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

    TEST_SIMPLE(test_obj_1);
    TEST_SIMPLE(test_alloc_basic);
    TEST_SIMPLE(test_alloc_aligned);

    return 0;
}
