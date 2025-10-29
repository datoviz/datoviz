/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Testing vklite                                                                               */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "_assertions.h"

#include "test_vklite.h"
#include "testing.h"



/*************************************************************************************************/
/*  Entry-point                                                                                  */
/*************************************************************************************************/

int test_vklite(TstSuite* suite)
{
    ANN(suite);

    const char* tags = "vklite";

    TEST_SIMPLE(test_vklite_commands_1);
    TEST_SIMPLE(test_vklite_sampler_1);
    TEST_SIMPLE(test_vklite_shader_1);
    TEST_SIMPLE(test_vklite_slots_1);
    TEST_SIMPLE(test_vklite_compute_1);
    TEST_SIMPLE(test_vklite_buffers_1);



    return 0;
}
