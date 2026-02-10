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

#include <inttypes.h>

#include "../macros.h"
#include "_assertions.h"
#include "_log.h"
#include "datoviz/vk/instance.h"
#include "test_vk.h"
#include "testing.h"
#include "vulkan_core.h"



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
    AT(count > 0);
    log_info("Found %u supported Vulkan layers:", count);

    for (uint32_t i = 0; i < count; i++)
    {
        log_info("  [%02u] %s", i, layers[i]);
    }

    // Free.
    dvz_instance_destroy(&instance);

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
    AT(count > 0);
    log_info("Found %u supported Vulkan instance extensions:", count);

    for (uint32_t i = 0; i < count; i++)
    {
        log_info("  [%02u] %s", i, extensions[i]);
    }

    // Free the array of strings.
    dvz_instance_destroy(&instance);

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

    // Probe capabilities before requesting optional extensions/layers.
    dvz_instance_probe_layers(&instance);
    dvz_instance_probe_extensions(&instance);

    bool has_validation = dvz_instance_has_layer(&instance, "VK_LAYER_KHRONOS_validation");
    bool has_debug_utils = dvz_instance_has_extension(&instance, VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    bool has_portability =
        dvz_instance_has_extension(&instance, VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);

    if (has_debug_utils)
    {
        dvz_instance_request_extension(&instance, VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    AT(instance.req_extension_count == (has_debug_utils ? 1 : 0));
    AT(instance.req_layer_count == 0);

    // Create the instance.
    int rc = dvz_instance_create(&instance, VK_API_VERSION_1_3);
    AT(rc == 0);

    uint32_t expected_layer_count = has_validation ? 1 : 0;
    uint32_t expected_ext_count = (has_debug_utils ? 1 : 0) + (has_portability ? 1 : 0);
    AT(instance.req_layer_count == expected_layer_count);
    AT(instance.req_extension_count == expected_ext_count);

    // Get Vulkan instance handle.
    VkInstance vk_instance = dvz_instance_handle(&instance);
    ANNVK(vk_instance);

    // Destroy the instance.
    dvz_instance_destroy(&instance);

    return 0;
}



int test_instance_creation_invalid_layer(TstSuite* suite, TstItem* tstitem)
{
    ANN(suite);
    ANN(tstitem);

    DvzInstance instance = {0};
    dvz_instance(&instance, 0);
    dvz_instance_info(&instance, "Invalid layer test", 1);
    dvz_instance_request_layer(&instance, "VK_LAYER_DATOVIZ_DOES_NOT_EXIST");

    int rc = dvz_instance_create(&instance, VK_API_VERSION_1_3);
    AT(rc != 0);
    AT(dvz_instance_handle(&instance) == VK_NULL_HANDLE);

    dvz_instance_destroy(&instance);
    return 0;
}
