/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Internal instance                                                                            */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "obj.h"
#include "datoviz/vk/instance.h"
#include "_gpu.h"



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzInstance
{
    DvzObject obj;
    bool is_heap_allocated;

    VkInstance vk_instance;
    uint32_t vk_version;

    // Instance creation structures.
    bool flags;
    bool portability;
    VkInstanceCreateInfo info_inst;
    VkDebugUtilsMessengerCreateInfoEXT info_debug;
    VkValidationFeaturesEXT validation_features;

    // Requested layers and extensions.
    uint32_t req_layer_count;
    uint32_t req_extension_count;
    char* req_layers[DVZ_MAX_REQ_LAYERS];
    char* req_extensions[DVZ_MAX_REQ_EXTENSIONS];

    // All supported instance layers and extensions.
    uint32_t layer_count;
    uint32_t extension_count;
    char** layers;
    char** extensions;

    // Application info.
    char* name;
    uint32_t version;

    // Validation.
    VkDebugUtilsMessengerEXT debug_messenger;
    uint32_t n_errors;

    // GPUs.
    uint32_t gpu_count;
    DvzGpu gpus[DVZ_MAX_GPUS];
};



/*************************************************************************************************/
/*  Internal instance API                                                                        */
/*************************************************************************************************/

void dvz_instance(DvzInstance* instance, int flags);
void dvz_instance_info(DvzInstance* instance, const char* name, uint32_t version);
void dvz_instance_portability(DvzInstance* instance);
void dvz_instance_validation_pre(DvzInstance* instance);
void dvz_instance_validation_post(DvzInstance* instance);
int dvz_instance_build(DvzInstance* instance, uint32_t vk_version);
void dvz_instance_request_layer(DvzInstance* instance, const char* layer);
void dvz_instance_request_extension(DvzInstance* instance, const char* extension);
