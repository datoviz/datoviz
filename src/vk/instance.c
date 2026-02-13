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

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <volk.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_compat.h"
#include "_log.h"
#include "datoviz/common/obj.h"
#include "datoviz/vk/instance.h"
#include "macros.h"
#include "validation.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define DVZ_PORTABILITY_EXTENSION_NAME VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME
#define DVZ_LAYER_VALIDATION_NAME      "VK_LAYER_KHRONOS_validation"

// Consistency check.
#define MAX_COUNT 1024



static bool _volk_initialized = false;
static bool _volk_ready = false;

/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Initialize Volk once and report whether Vulkan loader entry points are available.
 *
 * @return true when Volk initialization succeeds
 */
static bool _dvz_volk_init(void)
{
    if (_volk_initialized)
    {
        return _volk_ready;
    }

    log_trace("initializing volk");
    VkResult res = volkInitialize();
    _volk_initialized = true;
    if (res != VK_SUCCESS)
    {
        check_result(res);
        _volk_ready = false;
        return false;
    }

    _volk_ready = true;
    return true;
}



/**
 * Return whether a string is already present in a fixed-size string list.
 *
 * @param count number of strings in the list
 * @param values list of string values
 * @param value queried string value
 * @return true when the string exists in the list
 */
static bool _dvz_has_config_string(uint32_t count, const char* const* values, const char* value)
{
    ANN(values);
    ANN(value);
    for (uint32_t i = 0; i < count; i++)
    {
        if (values[i] != NULL && strcmp(values[i], value) == 0)
        {
            return true;
        }
    }
    return false;
}



/*************************************************************************************************/
/*  Instance                                                                                     */
/*************************************************************************************************/

/**
 * Return default configuration values for creating an instance.
 *
 * @return initialized instance configuration with sensible defaults
 */
DvzInstanceConfig dvz_instance_default_config(void)
{
    INIT(DvzInstanceConfig, cfg);
    cfg.vk_version = VK_API_VERSION_1_3;
    cfg.portability = true;
    return cfg;
}



/**
 * Add an instance layer request to a configuration.
 *
 * @param cfg the instance configuration
 * @param layer layer name to request
 * @return true when the request has been appended
 */
bool dvz_instance_config_request_layer(DvzInstanceConfig* cfg, const char* layer)
{
    ANN(cfg);
    ANN(layer);
    if (cfg->layer_count >= DVZ_MAX_REQ_LAYERS)
    {
        log_warn("too many requested instance layers");
        return false;
    }
    if (_dvz_has_config_string(cfg->layer_count, cfg->layers, layer))
    {
        return false;
    }
    cfg->layers[cfg->layer_count++] = layer;
    return true;
}



/**
 * Add an instance extension request to a configuration.
 *
 * @param cfg the instance configuration
 * @param extension extension name to request
 * @return true when the request has been appended
 */
bool dvz_instance_config_request_extension(DvzInstanceConfig* cfg, const char* extension)
{
    ANN(cfg);
    ANN(extension);
    if (cfg->extension_count >= DVZ_MAX_REQ_EXTENSIONS)
    {
        log_warn("too many requested instance extensions");
        return false;
    }
    if (_dvz_has_config_string(cfg->extension_count, cfg->extensions, extension))
    {
        return false;
    }
    cfg->extensions[cfg->extension_count++] = extension;
    return true;
}



/**
 * Create an instance from configuration in one step.
 *
 * @param cfg instance configuration
 * @return created instance, or `NULL` on failure
 */
DvzInstance* dvz_instance_create_from_config(const DvzInstanceConfig* cfg)
{
    ANN(cfg);
    DvzInstance* instance = (DvzInstance*)dvz_calloc(1, sizeof(DvzInstance));
    if (instance == NULL)
    {
        log_error("unable to allocate instance");
        return NULL;
    }

    dvz_instance(instance, cfg->flags);
    instance->is_heap_allocated = true;
    instance->portability = cfg->portability;
    dvz_instance_info(instance, cfg->app_name, cfg->app_version);

    for (uint32_t i = 0; i < cfg->layer_count; i++)
    {
        dvz_instance_request_layer(instance, cfg->layers[i]);
    }
    for (uint32_t i = 0; i < cfg->extension_count; i++)
    {
        dvz_instance_request_extension(instance, cfg->extensions[i]);
    }

    uint32_t vk_version = cfg->vk_version;
    if (vk_version == 0)
    {
        vk_version = VK_API_VERSION_1_3;
    }

    if (dvz_instance_create(instance, vk_version) != 0)
    {
        dvz_instance_destroy(instance);
        return NULL;
    }
    return instance;
}



void dvz_instance(DvzInstance* instance, int flags)
{
    ANN(instance);
    _dvz_volk_init();
    instance->flags = flags;
    instance->is_heap_allocated = false;
    instance->portability = true;
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
    ANNVK(instance->vk_instance);

    // Create debug messenger.
    VK_CHECK_RESULT(vkCreateDebugUtilsMessengerEXT(
        instance->vk_instance, &instance->info_debug, NULL, &instance->debug_messenger));
}



int dvz_instance_create(DvzInstance* instance, uint32_t vk_version)
{
    ANN(instance);
    instance->vk_version = vk_version;
    if (!_dvz_volk_init())
    {
        log_error("cannot create Vulkan instance because Volk initialization failed");
        return 1;
    }

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
    if (has_portability && instance->portability)
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
    VK_RETURN_RESULT(vkCreateInstance(&instance->info_inst, NULL, &instance->vk_instance));
    if (out)
        return out;

    volkLoadInstance(instance->vk_instance);
    ANNVK(instance->vk_instance);
    dvz_obj_created(&instance->obj);
    log_trace("Vulkan instance created");

    // Validation.
    if (has_validation)
    {
        dvz_instance_validation_post(instance);
    }

    return out;
}



VkInstance dvz_instance_handle(DvzInstance* instance)
{
    ANN(instance);
    return instance->vk_instance;
}



void dvz_instance_destroy(DvzInstance* instance)
{
    ANN(instance);
    bool is_heap_allocated = instance->is_heap_allocated;

    DVZ_FREE_STRING_CONTAINER(instance->layer_count, instance->layers)
    DVZ_FREE_STRING_CONTAINER(instance->extension_count, instance->extensions)

    // NOTE: free the strings in these arrays, but not the arrays themselves are they are not on
    // the heap.
    dvz_free_strings(instance->req_layer_count, instance->req_layers);
    dvz_free_strings(instance->req_extension_count, instance->req_extensions);

    DvzGpu* gpu = NULL;
    for (uint32_t i = 0; i < instance->gpu_count; i++)
    {
        gpu = &instance->gpus[i];
        ANN(gpu);
        if (gpu->extension_count > 0)
        {
            ANN(gpu->extensions);
            DVZ_FREE_STRING_CONTAINER(gpu->extension_count, gpu->extensions);
        }
    }

    dvz_free((char*)instance->name);

    VkInstance vki = instance->vk_instance;
    if (vki != VK_NULL_HANDLE)
    {
        // Destroy debug messenger.
        if (instance->debug_messenger != VK_NULL_HANDLE)
        {
            log_trace("destroy debug utils messenger");
            vkDestroyDebugUtilsMessengerEXT(vki, instance->debug_messenger, NULL);
        }

        // Destroy Vulkan instance.
        log_trace("destroy instance");
        vkDestroyInstance(vki, NULL);
    }

    dvz_obj_destroyed(&instance->obj);

    if (is_heap_allocated)
    {
        dvz_free(instance);
    }
}



/*************************************************************************************************/
/*  Layers                                                                                       */
/*************************************************************************************************/

void dvz_instance_probe_layers(DvzInstance* instance)
{
    ANN(instance);

    if (!_dvz_volk_init())
    {
        log_error("cannot probe Vulkan layers because Volk initialization failed");
        return;
    }

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

    if (!_dvz_volk_init())
    {
        log_error("cannot probe Vulkan instance extensions because Volk initialization failed");
        return;
    }

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
    log_trace("request instance extensions %s", extension);

    if (!dvz_strings_contains(instance->req_extension_count, instance->req_extensions, extension))
    {
        instance->req_extensions[instance->req_extension_count++] = dvz_strdup(extension);
    }
}
