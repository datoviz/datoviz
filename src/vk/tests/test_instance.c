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
#include "../validation.h"
#include "_assertions.h"
#include "_log.h"
#include "datoviz/vk/instance.h"
#include "test_vk.h"
#include "testing.h"
#include "vulkan_core.h"



/*************************************************************************************************/
/*  Instance tests                                                                               */
/*************************************************************************************************/

int test_instance_layers(TstContext* suite, const TstCase* tstitem)
{
    ANN(suite);
    ANN(tstitem);

    DvzInstanceConfig cfg = dvz_instance_config();
    cfg.flags = 0;
    DvzInstance* instance = dvz_instance_create(&cfg);
    AT(instance != NULL);
    dvz_instance_probe_layers(instance);

    // Call the function under test.
    uint32_t count = 0;
    char** layers = dvz_instance_supported_layers(instance, &count);
    AT(count > 0);
    log_debug("Found %u supported Vulkan layers:", count);

    for (uint32_t i = 0; i < count; i++)
    {
        log_debug("  [%02u] %s", i, layers[i]);
    }

    // Free.
    dvz_instance_destroy(instance);

    return 0;
}



int test_instance_extensions(TstContext* suite, const TstCase* tstitem)
{
    ANN(suite);
    ANN(tstitem);

    DvzInstanceConfig cfg = dvz_instance_config();
    cfg.flags = 0;
    DvzInstance* instance = dvz_instance_create(&cfg);
    AT(instance != NULL);
    dvz_instance_probe_extensions(instance);

    // Call the function under test.
    uint32_t count = 0;
    char** extensions = dvz_instance_supported_extensions(instance, &count);
    AT(count > 0);
    log_debug("Found %u supported Vulkan instance extensions:", count);

    for (uint32_t i = 0; i < count; i++)
    {
        log_debug("  [%02u] %s", i, extensions[i]);
    }

    // Free the array of strings.
    dvz_instance_destroy(instance);

    return 0;
}



int test_instance_creation(TstContext* suite, const TstCase* tstitem)
{
    ANN(suite);
    ANN(tstitem);

    DvzInstanceConfig cfg = dvz_instance_config();
    cfg.flags = DVZ_INSTANCE_VALIDATION_FLAGS;
    cfg.app_name = "Instance test";
    cfg.app_version = 42;
    AT(dvz_instance_config_request_extension(&cfg, VK_EXT_DEBUG_UTILS_EXTENSION_NAME));
    DvzInstance* instance = dvz_instance_create(&cfg);
    AT(instance != NULL);

    // Get Vulkan instance handle.
    VkInstance vk_instance = dvz_instance_handle(instance);
    ANNVK(vk_instance);

    // Destroy the instance.
    dvz_instance_destroy(instance);

    return 0;
}



int test_instance_validation_features(TstContext* suite, const TstCase* tstitem)
{
    ANN(suite);
    ANN(tstitem);

    AT(tst_unsetenv("DVZ_VK_DEBUG_PRINTF") == 0);
    VkValidationFeaturesEXT features = {0};
    _fill_validation_features(&features);
    AT(features.sType == VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT);
    AT(features.enabledValidationFeatureCount == 2);
    AT(features.pEnabledValidationFeatures[0] == VK_VALIDATION_FEATURE_ENABLE_BEST_PRACTICES_EXT);
    AT(
        features.pEnabledValidationFeatures[1] ==
        VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT);

    AT(tst_setenv("DVZ_VK_DEBUG_PRINTF", "1") == 0);
    _fill_validation_features(&features);
    AT(features.enabledValidationFeatureCount == 3);
    AT(
        features.pEnabledValidationFeatures[2] ==
        VK_VALIDATION_FEATURE_ENABLE_DEBUG_PRINTF_EXT);
    AT(tst_unsetenv("DVZ_VK_DEBUG_PRINTF") == 0);
    return 0;
}



int test_instance_invalid_layer(TstContext* suite, const TstCase* tstitem)
{
    ANN(suite);
    ANN(tstitem);

    DvzInstanceConfig cfg = dvz_instance_config();
    cfg.flags = 0;
    cfg.app_name = "Invalid layer test";
    cfg.app_version = 1;
    AT(dvz_instance_config_request_layer(&cfg, "VK_LAYER_DATOVIZ_DOES_NOT_EXIST"));
    DvzInstance* instance = NULL;
    AT_EXPECTED_ERROR(suite, (instance = dvz_instance_create(&cfg)) == NULL);
    return 0;
}
