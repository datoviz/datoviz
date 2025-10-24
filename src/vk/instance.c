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

// Consistency check.
#define MAX_COUNT 1024



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

    dvz_instance_request_extension(instance, DVZ_PORTABILITY_EXTENSION_NAME);
    instance->info_inst.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
}



void dvz_instance_validation_pre(DvzInstance* instance)
{
    ANN(instance);

    // Prepare the instance creation for validation support.

    // Add the validation layer and debug extension.
    dvz_instance_request_layer(instance, DVZ_LAYER_VALIDATION_NAME);
    dvz_instance_request_extension(instance, VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

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

    dvz_instance_probe_extensions(instance);
    dvz_instance_probe_layers(instance);

    // Whether the instance creation supports portability enumeration.
    bool has_portability = dvz_instance_has_extension(instance, DVZ_PORTABILITY_EXTENSION_NAME);

    // Whether the validation layer is supported and requested.
    bool can_validation = dvz_instance_has_layer(instance, DVZ_LAYER_VALIDATION_NAME);
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
    instance->info_inst.enabledExtensionCount = instance->req_extension_count;
    instance->info_inst.ppEnabledExtensionNames = (const char* const*)instance->req_extensions;

    // Enabled layers.
    instance->info_inst.enabledLayerCount = instance->req_layer_count;
    instance->info_inst.ppEnabledLayerNames = (const char* const*)instance->req_layers;

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

    dvz_free_strings(instance->layer_count, instance->layers);
    dvz_free(instance->layers);

    dvz_free_strings(instance->extension_count, instance->extensions);
    dvz_free(instance->extensions);

    // NOTE: free the strings in these arrays, but not the arrays themselves are they are not on
    // the heap.
    dvz_free_strings(instance->req_layer_count, instance->req_layers);
    dvz_free_strings(instance->req_extension_count, instance->req_extensions);

    for (uint32_t i = 0; i < instance->gpu_count; i++)
    {
        if (instance->gpus[i].extension_count > 0)
        {
            ANN(instance->gpus[i].extensions);
            dvz_free_strings(instance->gpus[i].extension_count, instance->gpus[i].extensions);
            dvz_free(instance->gpus[i].extensions);
        }
    }

    dvz_free((char*)instance->name);
}



/*************************************************************************************************/
/*  Layers                                                                                       */
/*************************************************************************************************/

void dvz_instance_probe_layers(DvzInstance* instance)
{
    ANN(instance);

    if (instance->layer_count > 0)
        return;

    // Get the number of instance layers.
    VkResult res = vkEnumerateInstanceLayerProperties(&instance->layer_count, NULL);
    if (res != VK_SUCCESS || instance->layer_count == 0)
        return;

    // Get the names of the instance layers.
    ASSERT(instance->layer_count > 0);
    ASSERT(instance->layer_count < MAX_COUNT); // consistency check
    VkLayerProperties* props =
        (VkLayerProperties*)dvz_calloc((size_t)instance->layer_count, sizeof(VkLayerProperties));
    if (!props)
        return;

    res = vkEnumerateInstanceLayerProperties(&instance->layer_count, props);
    if (res != VK_SUCCESS)
    {
        dvz_free(props);
        return;
    }

    // Allocate the array of strings.
    instance->layers = (char**)dvz_calloc((size_t)instance->layer_count, sizeof(char*));
    for (uint32_t i = 0; i < instance->layer_count; i++)
    {
        // Allocate the string.
        instance->layers[i] = (char*)dvz_calloc(VK_MAX_EXTENSION_NAME_SIZE, sizeof(char));
        ANN(instance->layers[i]);

        // Fill in the string.
        (void)dvz_snprintf(
            instance->layers[i], VK_MAX_EXTENSION_NAME_SIZE, "%s", props[i].layerName);
    }

    dvz_free(props);
}



char** dvz_instance_supported_layers(DvzInstance* instance, uint32_t* count)
{
    ANN(instance);
    ANN(count);
    *count = instance->layer_count;
    return instance->layers;
}



bool dvz_instance_has_layer(DvzInstance* instance, const char* layer)
{
    ANN(instance);
    ANN(layer);
    return dvz_strings_contains(instance->layer_count, instance->layers, layer);
}



void dvz_instance_request_layer(DvzInstance* instance, const char* layer)
{
    ANN(instance);
    ANN(layer);

    ANN(instance->req_layers);
    ASSERT(instance->req_layer_count < DVZ_MAX_REQ_LAYERS - 1);

    if (!dvz_strings_contains(instance->req_layer_count, instance->req_layers, layer))
    {
        instance->req_layers[instance->req_layer_count++] = dvz_strdup(layer);
    }
}



/*************************************************************************************************/
/*  Extensions                                                                                   */
/*************************************************************************************************/

void dvz_instance_probe_extensions(DvzInstance* instance)
{
    ANN(instance);

    if (instance->extension_count > 0)
        return;

    // Get the number of instance extensions.
    VkResult res = vkEnumerateInstanceExtensionProperties(NULL, &instance->extension_count, NULL);
    if (res != VK_SUCCESS || instance->extension_count == 0)
        return;

    // Get the names of the instance extensions.
    ASSERT(instance->extension_count > 0);
    ASSERT(instance->extension_count < MAX_COUNT); // consistency check
    VkExtensionProperties* props = (VkExtensionProperties*)dvz_calloc(
        (size_t)instance->extension_count, sizeof(VkExtensionProperties));
    if (!props)
        return;

    res = vkEnumerateInstanceExtensionProperties(NULL, &instance->extension_count, props);
    if (res != VK_SUCCESS)
    {
        dvz_free(props);
        return;
    }

    // Allocate the array of strings.
    instance->extensions = (char**)dvz_calloc((size_t)instance->extension_count, sizeof(char*));
    for (uint32_t i = 0; i < instance->extension_count; i++)
    {
        // Allocate the string.
        instance->extensions[i] = (char*)dvz_calloc(VK_MAX_EXTENSION_NAME_SIZE, sizeof(char));
        ANN(instance->extensions[i]);

        // Fill in the string.
        (void)dvz_snprintf(
            instance->extensions[i], VK_MAX_EXTENSION_NAME_SIZE, "%s", props[i].extensionName);
    }

    dvz_free(props);
}



char** dvz_instance_supported_extensions(DvzInstance* instance, uint32_t* count)
{
    ANN(instance);
    ANN(count);
    *count = instance->extension_count;
    return instance->extensions;
}



bool dvz_instance_has_extension(DvzInstance* instance, const char* extension)
{
    ANN(instance);
    ANN(extension);
    return dvz_strings_contains(instance->extension_count, instance->extensions, extension);
}



void dvz_instance_request_extension(DvzInstance* instance, const char* extension)
{
    ANN(instance);
    ANN(extension);

    ANN(instance->req_extensions);
    ASSERT(instance->req_extension_count < DVZ_MAX_REQ_EXTENSIONS - 1);

    if (!dvz_strings_contains(instance->req_extension_count, instance->req_extensions, extension))
    {
        instance->req_extensions[instance->req_extension_count++] = dvz_strdup(extension);
    }
}
