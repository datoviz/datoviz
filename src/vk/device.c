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
#include "datoviz/common/obj.h"
#include "datoviz/vk/device.h"
#include "macros.h"
#include "types.h"
#include "validation.h"
#include "vulkan/vulkan_core.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define DVZ_MAX_LAYERS                 256
#define DVZ_MAX_EXTENSIONS             256
#define DVZ_PORTABILITY_EXTENSION_NAME VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME



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
    ASSERT(*count < DVZ_MAX_LAYERS); // consistency check
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

    ASSERT(*count < DVZ_MAX_EXTENSIONS); // consistency check

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



bool dvz_instance_has_extension(const char* extension)
{
    if (extension == NULL)
        return false;

    uint32_t count = 0;

    // Get the number of instance extensions.
    VkResult res = vkEnumerateInstanceExtensionProperties(NULL, &count, NULL);
    if (res != VK_SUCCESS || count == 0)
        return 0;

    ASSERT(count < DVZ_MAX_EXTENSIONS); // consistency check

    // Allocate and retrieve the extension properties.
    VkExtensionProperties* props =
        (VkExtensionProperties*)dvz_calloc((size_t)count, sizeof(VkExtensionProperties));
    if (!props)
        return 0;
    ANN(props);

    for (uint32_t i = 0; i < count; i++)
    {
        ANN(props[i].extensionName);
        if (strncmp(props[i].extensionName, extension, VK_MAX_EXTENSION_NAME_SIZE) == 0)
        {
            dvz_free(props);
            return true;
        }
    }
    dvz_free(props);
    return false;
}



void dvz_instance_layer(DvzInstance* instance, const char* layer)
{
    ANN(instance);
    ANN(layer);
    instance->layers[instance->layer_count++] = dvz_strdup(layer);
}



void dvz_instance_layers(DvzInstance* instance, uint32_t count, const char** layers)
{
    ANN(instance);
    if (count > 0)
        ANN(layers);

    instance->layer_count = count;
    instance->layers = dvz_copy_strings(count, layers);
}



void dvz_instance_extension(DvzInstance* instance, const char* extension)
{
    ANN(instance);
    ANN(extension);
    instance->extensions[instance->ext_count++] = dvz_strdup(extension);
}



void dvz_instance_extensions(DvzInstance* instance, uint32_t count, const char** extensions)
{
    ANN(instance);
    if (count > 0)
        ANN(extensions);

    instance->ext_count = count;
    instance->extensions = dvz_copy_strings(count, extensions);
}



void dvz_instance_portability(DvzInstance* instance)
{
    ANN(instance);

    if (dvz_instance_has_extension(DVZ_PORTABILITY_EXTENSION_NAME))
    {
        dvz_instance_extension(instance, DVZ_PORTABILITY_EXTENSION_NAME);
        instance->create_flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
    }
}



void dvz_instance_info(DvzInstance* instance, const char* name, uint32_t version)
{
    ANN(instance);

    instance->obj.type = DVZ_OBJECT_TYPE_DEVICE;
    dvz_obj_init(&instance->obj);

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


    // Add portability enumeration extension and creation flag if "VK_KHR_portability_enumeration"
    // is in the supported instance extensions.
    dvz_instance_portability(instance);
    info_inst.flags |= instance->create_flags;


    // TODO: IF VALIDATION

    // Validation structures
    VkDebugUtilsMessengerCreateInfoEXT info_debug = {0};
    _fill_info_debug(&info_debug);
    info_debug.pUserData = &instance->n_errors;

    VkValidationFeaturesEXT features = {0};
    _fill_validation_features(&features);
    features.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&info_debug;

    info_inst.pNext = &features;



    // Create Vulkan instance.
    log_trace("creating Vulkan instance...");
    VkResult res = vkCreateInstance(&info_inst, NULL, &instance->vk_instance);
    check_result(res);

    VkInstance vki = instance->vk_instance;
    ASSERT(vki != VK_NULL_HANDLE);
    dvz_obj_created(&instance->obj);
    log_trace("Vulkan instance created");



    // Create debug messenger.
    LOAD_VK_FUNC(vki, vkCreateDebugUtilsMessengerEXT);
    vkCreateDebugUtilsMessengerEXT_d(vki, &info_debug, NULL, &instance->debug_messenger);



    return res;
}



uint32_t dvz_instance_gpus(DvzInstance* instance, DvzGpu* gpus)
{
    ANN(instance);

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

    VkInstance vki = instance->vk_instance;

    if (vki != VK_NULL_HANDLE)
    {
        if (instance->debug_messenger != NULL)
        {
            LOAD_VK_FUNC(vki, vkDestroyDebugUtilsMessengerEXT);

            vkDestroyDebugUtilsMessengerEXT_d(vki, instance->debug_messenger, NULL);
        }

        vkDestroyInstance(vki, NULL);
        dvz_obj_destroyed(&instance->obj);
    }

    dvz_free_strings(instance->ext_count, (char**)instance->extensions);
    dvz_free_strings(instance->layer_count, (char**)instance->layers);
    dvz_free((char*)instance->name);
}
