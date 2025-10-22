/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Device                                                                                      */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdint.h>

#include <vulkan/vulkan.h>

#include "_alloc.h"
#include "_compat.h"
#include "datoviz/common/macros.h"
#include "datoviz/vk/device.h"



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

DVZ_EXPORT char** dvz_instance_supported_layers(uint32_t* count)
{
    ANN(count);

    // Get the number of instance layers.
    VkResult res = vkEnumerateInstanceLayerProperties(count, NULL);
    if (res != VK_SUCCESS || *count == 0)
        return 0;

    // Get the names of the instance layers.
    ASSERT(*count < 1024); // consistency check
    VkLayerProperties* props =
        (VkLayerProperties*)dvz_calloc((size_t)*count, sizeof(VkLayerProperties));
    if (!props)
        return 0;

    res = vkEnumerateInstanceLayerProperties(count, props);
    if (res != VK_SUCCESS)
    {
        dvz_free(props);
        return 0;
    }

    // Allocate the array of strings.
    char** layers = (char**)dvz_calloc((size_t)*count, sizeof(char*));
    for (uint32_t i = 0; i < *count; i++)
    {
        // Allocate the string.
        layers[i] = (char*)dvz_calloc(VK_MAX_EXTENSION_NAME_SIZE, sizeof(char));
        ANN(layers[i]);

        // Fill in the string.
        (void)dvz_snprintf(layers[i], VK_MAX_EXTENSION_NAME_SIZE, "%s", props[i].layerName);
    }

    dvz_free(props);
    return layers;
}
