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

    TST_MODULE(suite, "ds");
    TST_GROUP("map");
    TST_CASE(test_map_1);
    TST_CASE(test_map_2);

    TST_GROUP("list");
    TST_CASE(test_list_1);
    TST_CASE(test_list_remove_pointer);

    return 0;
}
