/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Instance                                                                                     */
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
#include "datoviz/vk/instance.h"
#include "macros.h"
#include "types.h"
#include "validation.h"
#include "vulkan/vulkan_core.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define DVZ_PORTABILITY_EXTENSION_NAME VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME
#define DVZ_LAYER_VALIDATION_NAME      "VK_LAYER_KHRONOS_validation"



/*************************************************************************************************/
/*  Instance                                                                                     */
/*************************************************************************************************/

void dvz_instance(DvzInstance* instance, int flags)
{
    ANN(instance);
    instance->flags = flags;
    instance->obj.type = DVZ_OBJECT_TYPE_INSTANCE;
    dvz_obj_init(&instance->obj);
}



void dvz_instance_info(DvzInstance* instance, const char* name, uint32_t version)
{
    ANN(instance);

    if (name != NULL)
        instance->name = dvz_strdup(name);

    instance->version = version;
}



void dvz_instance_portability(DvzInstance* instance)
{
    ANN(instance);

    dvz_instance_extension(instance, DVZ_PORTABILITY_EXTENSION_NAME);
    instance->info_inst.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
}



void dvz_instance_validation_pre(DvzInstance* instance)
{
    ANN(instance);

    // Prepare the instance creation for validation support.

    // Add the validation layer and debug extension.
    dvz_instance_layer(instance, DVZ_LAYER_VALIDATION_NAME);
    dvz_instance_extension(instance, VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

    // Validation info debug.
    _fill_info_debug(&instance->info_debug);
    instance->info_debug.pUserData = &instance->n_errors;

    // Validation features.
    _fill_validation_features(&instance->validation_features);
    instance->validation_features.pNext =
        (VkDebugUtilsMessengerCreateInfoEXT*)&instance->info_debug;
    instance->info_inst.pNext = &instance->validation_features;
}



void dvz_instance_validation_post(DvzInstance* instance)
{
    ANN(instance);

    if (instance->vk_instance == VK_NULL_HANDLE)
    {
        log_warn("cannot set up validation layers after instance creation as creation failed");
        return;
    }
    ASSERT(instance->vk_instance != VK_NULL_HANDLE);

    // Create debug messenger.
    LOAD_VK_FUNC(instance->vk_instance, vkCreateDebugUtilsMessengerEXT);
    vkCreateDebugUtilsMessengerEXT_d(
        instance->vk_instance, &instance->info_debug, NULL, &instance->debug_messenger);
}



int dvz_instance_create(DvzInstance* instance, uint32_t vk_version)
{
    ANN(instance);
    instance->vk_version = vk_version;

    // Whether the instance creation supports portability enumeration.
    bool has_portability = dvz_instance_has_extension(DVZ_PORTABILITY_EXTENSION_NAME);

    // Whether the validation layer is supported and requested.
    bool can_validation = dvz_instance_has_layer(DVZ_LAYER_VALIDATION_NAME);
    bool wants_validation = (instance->flags & DVZ_INSTANCE_VALIDATION_FLAGS) != 0;
    if (!can_validation && wants_validation)
    {
        log_warn("validation layer is not supported");
    }
    bool has_validation = can_validation && wants_validation;

    // Prepare the creation of the Vulkan instance.
    VkApplicationInfo appInfo = {0};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = instance->name;
    appInfo.applicationVersion = instance->version;
    appInfo.apiVersion = instance->vk_version;

    instance->info_inst.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instance->info_inst.pApplicationInfo = &appInfo;

    // Add portability enumeration extension and creation flag if "VK_KHR_portability_enumeration"
    // is in the supported instance extensions.
    if (has_portability)
    {
        dvz_instance_portability(instance);
    }

    // Validation.
    if (has_validation)
    {
        dvz_instance_validation_pre(instance);
    }

    // Enabled instance extensions.
    instance->info_inst.enabledExtensionCount = instance->ext_count;
    instance->info_inst.ppEnabledExtensionNames = (const char* const*)instance->extensions;

    // Enabled layers.
    instance->info_inst.enabledLayerCount = instance->layer_count;
    instance->info_inst.ppEnabledLayerNames = (const char* const*)instance->layers;

    // Create Vulkan instance.
    log_trace("creating Vulkan instance...");
    VkResult res = vkCreateInstance(&instance->info_inst, NULL, &instance->vk_instance);
    check_result(res);
    ASSERT(instance->vk_instance != VK_NULL_HANDLE);
    dvz_obj_created(&instance->obj);
    log_trace("Vulkan instance created");

    // Validation.
    if (has_validation)
    {
        dvz_instance_validation_post(instance);
    }

    return res;
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

    dvz_free_strings(instance->ext_count, instance->extensions);
    dvz_free_strings(instance->layer_count, instance->layers);
    dvz_free((char*)instance->name);
}



/*************************************************************************************************/
/*  Layers                                                                                       */
/*************************************************************************************************/

char** dvz_instance_supported_layers(uint32_t* count)
{
    ANN(count);

    // Get the number of instance layers.
    VkResult res = vkEnumerateInstanceLayerProperties(count, NULL);
    if (res != VK_SUCCESS || *count == 0)
        return 0;

    // Get the names of the instance layers.
    ASSERT(*count < DVZ_MAX_LAYERS * 8); // consistency check
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



bool dvz_instance_has_layer(const char* layer)
{
    if (layer == NULL)
        return false;

    uint32_t count = 0;

    // Get the number of instance layers.
    VkResult res = vkEnumerateInstanceLayerProperties(&count, NULL);
    if (res != VK_SUCCESS || count == 0)
        return 0;

    ASSERT(count > 0);
    ASSERT(count < DVZ_MAX_LAYERS * 8); // consistency check

    // Allocate and retrieve the layer properties.
    VkLayerProperties* props =
        (VkLayerProperties*)dvz_calloc((size_t)count, sizeof(VkLayerProperties));
    if (!props)
        return 0;
    ANN(props);

    res = vkEnumerateInstanceLayerProperties(&count, props);
    if (res != VK_SUCCESS)
    {
        dvz_free(props);
        return 0;
    }

    for (uint32_t i = 0; i < count; i++)
    {
        ANN(props[i].layerName);
        if (strncmp(props[i].layerName, layer, VK_MAX_EXTENSION_NAME_SIZE) == 0)
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
    ANN(instance->layers);
    ASSERT(instance->layer_count < DVZ_MAX_LAYERS - 2);

    if (!dvz_strings_contains(instance->layer_count, instance->layers, layer))
    {
        instance->layers[instance->layer_count++] = dvz_strdup(layer);
    }
}



void dvz_instance_layers(DvzInstance* instance, uint32_t count, const char** layers)
{
    ANN(instance);
    if (count > 0)
        ANN(layers);
    if (count >= DVZ_MAX_LAYERS)
    {
        log_warn("too many instance layers");
        return;
    }

    if (instance->layer_count > 0)
        dvz_free_strings(instance->layer_count, instance->layers);

    instance->layer_count = count;
    dvz_copy_strings(count, layers, instance->layers);
}



/*************************************************************************************************/
/*  Extensions                                                                                   */
/*************************************************************************************************/

char** dvz_instance_supported_extensions(uint32_t* count)
{
    ANN(count);

    // Get the number of instance extensions.
    VkResult res = vkEnumerateInstanceExtensionProperties(NULL, count, NULL);
    if (res != VK_SUCCESS || *count == 0)
        return 0;

    ASSERT(*count < DVZ_MAX_EXTENSIONS * 8); // consistency check

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

    ASSERT(count < DVZ_MAX_EXTENSIONS * 8); // consistency check

    // Allocate and retrieve the extension properties.
    VkExtensionProperties* props =
        (VkExtensionProperties*)dvz_calloc((size_t)count, sizeof(VkExtensionProperties));
    if (!props)
        return 0;
    ANN(props);

    res = vkEnumerateInstanceExtensionProperties(NULL, &count, props);
    if (res != VK_SUCCESS)
    {
        dvz_free(props);
        return 0;
    }

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



void dvz_instance_extension(DvzInstance* instance, const char* extension)
{
    ANN(instance);
    ANN(extension);
    ANN(instance->extensions);
    ASSERT(instance->ext_count < DVZ_MAX_EXTENSIONS - 2);

    if (!dvz_strings_contains(instance->ext_count, instance->extensions, extension))
    {
        instance->extensions[instance->ext_count++] = dvz_strdup(extension);
    }
}



void dvz_instance_extensions(DvzInstance* instance, uint32_t count, const char** extensions)
{
    ANN(instance);
    if (count > 0)
        ANN(extensions);
    if (count >= DVZ_MAX_EXTENSIONS)
    {
        log_warn("too many instance extensions");
        return;
    }

    if (instance->ext_count > 0)
        dvz_free_strings(instance->ext_count, instance->extensions);

    instance->ext_count = count;
    dvz_copy_strings(count, extensions, instance->extensions);
}
