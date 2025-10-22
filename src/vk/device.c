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
#include "macros.h"
#include "vulkan/vulkan_core.h"



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzInstance
{
    VkInstance vk_instance;
    uint32_t vk_version;

    uint32_t layer_count;
    char** layers;

    uint32_t ext_count;
    char** extensions;

    char* name;
    uint32_t version;
};



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

char** dvz_instance_supported_layers(uint32_t* count)
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



char** dvz_instance_supported_extensions(uint32_t* count)
{
    ANN(count);

    // Get the number of instance extensions.
    VkResult res = vkEnumerateInstanceExtensionProperties(NULL, count, NULL);
    if (res != VK_SUCCESS || *count == 0)
        return 0;

    ASSERT(*count < 1024); // consistency check

    // Allocate and retrieve the extension properties.
    VkExtensionProperties* props =
        (VkExtensionProperties*)dvz_calloc((size_t)*count, sizeof(VkExtensionProperties));
    if (!props)
        return 0;

    res = vkEnumerateInstanceExtensionProperties(NULL, count, props);
    if (res != VK_SUCCESS)
    {
        dvz_free(props);
        return 0;
    }

    // Allocate the array of strings.
    char** extensions = (char**)dvz_calloc((size_t)*count, sizeof(char*));
    for (uint32_t i = 0; i < *count; i++)
    {
        // Allocate memory for each string.
        extensions[i] = (char*)dvz_calloc(VK_MAX_EXTENSION_NAME_SIZE, sizeof(char));
        ANN(extensions[i]);

        // Copy the extension name.
        (void)dvz_snprintf(
            extensions[i], VK_MAX_EXTENSION_NAME_SIZE, "%s", props[i].extensionName);
    }

    dvz_free(props);
    return extensions;
}



void dvz_instance_layers(DvzInstance* instance, uint32_t count, const char** layers)
{
    ANN(instance);
    if (count > 0)
        ANN(layers);

    instance->layer_count = count;
    instance->layers = dvz_copy_strings(count, layers);
}



void dvz_instance_extensions(DvzInstance* instance, uint32_t count, const char** extensions)
{
    ANN(instance);
    if (count > 0)
        ANN(extensions);

    instance->layer_count = count;
    instance->extensions = dvz_copy_strings(count, extensions);
}



void dvz_instance_info(DvzInstance* instance, const char* name, uint32_t version)
{
    ANN(instance);
    if (name != NULL)
        instance->name = dvz_strdup(name);

    instance->version = version;
}



int dvz_instance_create(DvzInstance* instance, uint32_t vk_version)
{
    ANN(instance);
    instance->vk_version = vk_version;


    // Prepare the creation of the Vulkan instance.
    VkApplicationInfo appInfo = {0};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = instance->name;
    appInfo.applicationVersion = instance->version;
    appInfo.apiVersion = instance->vk_version;

    VkInstanceCreateInfo info_inst = {0};
    info_inst.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    info_inst.pApplicationInfo = &appInfo;

    // Enabled instance extensions.
    info_inst.enabledExtensionCount = instance->ext_count;
    info_inst.ppEnabledExtensionNames = (const char* const*)instance->extensions;

    log_trace("creating instance");
    VkResult res = vkCreateInstance(&info_inst, NULL, &instance->vk_instance);
    check_result(res);
    return res;
}



uint32_t dvz_instance_gpus(DvzInstance* instance, DvzGpu* gpus)
{
    ANN(instance); //

    return 0;
}



VkInstance dvz_instance_handle(DvzInstance* instance)
{
    ANN(instance);
    return instance->vk_instance;
}



void dvz_instance_destroy(DvzInstance* instance)
{
    ANN(instance);

    if (instance->vk_instance != VK_NULL_HANDLE)
    {
        vkDestroyInstance(instance->vk_instance, NULL);
    }

    dvz_free_strings(instance->ext_count, (char**)instance->extensions);
    dvz_free_strings(instance->layer_count, (char**)instance->layers);
    dvz_free((char*)instance->name);
}
