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
    dvz_free(layers);

    log_info("Test completed successfully.");

    return 0;
}



int test_instance_extensions(TstSuite* suite, TstItem* tstitem)
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
    dvz_free(extensions);

    log_info("Test completed successfully.");

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

    // Enable validation layer and debug extension.
    dvz_instance_layers(&instance, 1, (const char*[]){"VK_LAYER_KHRONOS_synchronization2"});
    dvz_instance_extensions(&instance, 1, (const char*[]){VK_EXT_DEBUG_UTILS_EXTENSION_NAME});

    // Add one layer.
    dvz_instance_layer(&instance, "VK_LAYER_LUNARG_api_dump");

    // Add one extension.
    dvz_instance_extension(&instance, VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);

    AT(instance.ext_count == 2);
    AT(instance.layer_count == 2);

    // Create the instance.
    dvz_instance_create(&instance, VK_API_VERSION_1_3);

    AT(instance.ext_count == 2);   // debug extension is already there
    AT(instance.layer_count == 3); // add validation layer


    // Get Vulkan instance handle.
    VkInstance vk_instance = dvz_instance_handle(&instance);
    AT(vk_instance != VK_NULL_HANDLE);


    // Destroy the instance.
    dvz_instance_destroy(&instance);

    return 0;
}
