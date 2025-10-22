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



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

DVZ_EXPORT uint32_t dvz_instance_supported_layers(char** layers)
{
    ANN(layers);

    // Get the number of instance layers.
    uint32_t count = 0;
    VkResult res = vkEnumerateInstanceLayerProperties(&count, NULL);
    if (res != VK_SUCCESS || count == 0)
        return 0;

    // Get the names of the instance layers.
    VkLayerProperties* props =
        (VkLayerProperties*)dvz_malloc(sizeof(VkLayerProperties) * (size_t)count);
    if (!props)
        return 0;

    res = vkEnumerateInstanceLayerProperties(&count, props);
    if (res != VK_SUCCESS)
    {
        dvz_free(props);
        return 0;
    }

    for (uint32_t i = 0; i < count; i++)
    {
        ANN(layers[i]);
        (void)dvz_snprintf(layers[i], VK_MAX_EXTENSION_NAME_SIZE, "%s", props[i].layerName);
    }

    dvz_free(props);
    return count;
}
