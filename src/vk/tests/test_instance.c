/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Testing instance                                                                             */
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
#include "datoviz/vk/instance.h"
#include "test_vk.h"
#include "testing.h"



/*************************************************************************************************/
/*  Instance tests                                                                               */
/*************************************************************************************************/

int test_instance_layers(TstSuite* suite, TstItem* tstitem)
{
    ANN(suite);
    ANN(tstitem);

    DvzInstance instance = {0};
    dvz_instance(&instance, 0);
    dvz_instance_probe_layers(&instance);

    // Call the function under test.
    uint32_t count = 0;
    char** layers = dvz_instance_supported_layers(&instance, &count);
    log_info("Found %u supported Vulkan layers:", count);

    for (uint32_t i = 0; i < count; i++)
    {
        log_info("  [%02u] %s", i, layers[i]);
    }

    // Free.
    dvz_free_strings(count, layers);
    dvz_free(layers);

    return 0;
}



int test_instance_extensions(TstSuite* suite, TstItem* tstitem)
{
    ANN(suite);
    ANN(tstitem);

    DvzInstance instance = {0};
    dvz_instance(&instance, 0);
    dvz_instance_probe_extensions(&instance);

    // Call the function under test.
    uint32_t count = 0;
    char** extensions = dvz_instance_supported_extensions(&instance, &count);
    log_info("Found %u supported Vulkan instance extensions:", count);

    for (uint32_t i = 0; i < count; i++)
    {
        log_info("  [%02u] %s", i, extensions[i]);
    }

    // Free the array of strings.
    dvz_free_strings(count, extensions);
    dvz_free(extensions);

    return 0;
}



int test_instance_creation(TstSuite* suite, TstItem* tstitem)
{
    ANN(suite);
    ANN(tstitem);

    // Initialize the instance structure.
    DvzInstance instance = {0};
    dvz_instance(&instance, DVZ_INSTANCE_VALIDATION_FLAGS);
    dvz_instance_info(&instance, "Instance test", 42);

    // Add one layer.
    dvz_instance_request_layer(&instance, "VK_LAYER_KHRONOS_synchronization2");

    // Add one extension.
    dvz_instance_request_extension(&instance, VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

    AT(instance.req_extension_count == 1);
    AT(instance.req_layer_count == 1);

    // Create the instance.
    dvz_instance_create(&instance, VK_API_VERSION_1_3);

    AT(instance.req_layer_count == 2);     // add validation layer
    AT(instance.req_extension_count == 2); // debug extension is already there, but add portability


    // Get Vulkan instance handle.
    VkInstance vk_instance = dvz_instance_handle(&instance);
    AT(vk_instance != VK_NULL_HANDLE);


    // Destroy the instance.
    dvz_instance_destroy(&instance);

    return 0;
}
