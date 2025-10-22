/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Testing device                                                                              */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <float.h>
#include <inttypes.h>
#include <stdbool.h>
#include <vulkan/vulkan_core.h>

#include "../types.h"
#include "_alloc.h"
#include "_assertions.h"
#include "_log.h"
#include "datoviz/vk/device.h"
#include "test_vk.h"
#include "testing.h"



/*************************************************************************************************/
/*  <Title> tests                                                                                */
/*************************************************************************************************/

int test_device_layers(TstSuite* suite, TstItem* tstitem)
{
    ANN(suite);
    ANN(tstitem);

    log_info("Testing dvz_instance_supported_layers()...");

    // Call the function under test.
    uint32_t count = 0;
    char** layers = dvz_instance_supported_layers(&count);
    log_info("Found %u supported Vulkan layers:", count);

    for (uint32_t i = 0; i < count; i++)
    {
        log_info("  [%02u] %s", i, layers[i]);
    }

    // Free.
    dvz_free_strings(count, layers);

    log_info("Test completed successfully.");

    return 0;
}



int test_device_extensions(TstSuite* suite, TstItem* tstitem)
{
    ANN(suite);
    ANN(tstitem);

    log_info("Testing dvz_instance_supported_extensions()...");

    // Call the function under test.
    uint32_t count = 0;
    char** extensions = dvz_instance_supported_extensions(&count);
    log_info("Found %u supported Vulkan instance extensions:", count);

    for (uint32_t i = 0; i < count; i++)
    {
        log_info("  [%02u] %s", i, extensions[i]);
    }

    // Free the array of strings.
    dvz_free_strings(count, extensions);

    log_info("Test completed successfully.");

    return 0;
}



int test_device_instance(TstSuite* suite, TstItem* tstitem)
{
    ANN(suite);
    ANN(tstitem);

    DvzInstance instance = {0};
    dvz_instance_info(&instance, "Instance test", 42);

    VkInstance vk_instance = dvz_instance_handle(&instance);
    dvz_instance_create(&instance, VK_API_VERSION_1_3);
    dvz_instance_destroy(&instance);

    return 0;
}
