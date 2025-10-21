/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Testing data structures                                                                      */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "_assertions.h"

#include "test_ds.h"
#include "testing.h"



/*************************************************************************************************/
/*  Entry-point                                                                                  */
/*************************************************************************************************/

int test_ds(TstSuite* suite)
{
    ANN(suite);

    const char* tags = "ds";

    TEST_SIMPLE(test_map_1);
   TEST_SIMPLE(test_map_2);
   TEST_SIMPLE(test_list_1);
    TEST_SIMPLE(test_list_remove_pointer);

    return 0;
}
