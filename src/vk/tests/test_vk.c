/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Testing Vulkan                                                                               */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "_assertions.h"

#include "test_vk.h"
#include "testing.h"



/*************************************************************************************************/
/*  Entry-point                                                                                  */
/*************************************************************************************************/

int test_vk(TstSuite* suite)
{
    ANN(suite);

    const char* tags = "vk";

    TEST_SIMPLE(test_instance_layers);
    TEST_SIMPLE(test_instance_extensions);
    TEST_SIMPLE(test_instance_creation);

    TEST_SIMPLE(test_gpu_props);
    TEST_SIMPLE(test_gpu_memprops);
    TEST_SIMPLE(test_gpu_features);
    TEST_SIMPLE(test_gpu_extensions);

    TEST_SIMPLE(test_queues_caps);
    TEST_SIMPLE(test_queues_basic);
    TEST_SIMPLE(test_queues_multiple);
    TEST_SIMPLE(test_queues_tie_break);
    TEST_SIMPLE(test_queues_no_optional);
    TEST_SIMPLE(test_queues_video_roles);
    TEST_SIMPLE(test_queue_from_role);
    TEST_SIMPLE(test_queue_supports);



    return 0;
}
