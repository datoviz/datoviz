/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/* Vulkan types                                                                                  */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

#include "vulkan/vulkan_core.h"
typedef struct DvzInstance DvzInstance;
typedef struct DvzGpu DvzGpu;



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdint.h>

#include <volk.h>

#include "datoviz/common/obj.h"

MUTE_ON
#define VMA_EXTERNAL_MEMORY 1
#include "vk_mem_alloc.h"
MUTE_OFF

#include "datoviz/common/obj.h"
#include "datoviz/vk/queues.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

// Maximum number of requested layers/extensions.
#define DVZ_MAX_REQ_LAYERS     32
#define DVZ_MAX_REQ_EXTENSIONS 256
#define DVZ_MAX_GPUS           8



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzVma DvzVma;
typedef struct DvzAllocation DvzAllocation;
typedef struct DvzDevice DvzDevice;
typedef struct DvzGpu DvzGpu;
typedef struct DvzInstance DvzInstance;
typedef struct DvzBootstrap DvzBootstrap;



/*************************************************************************************************/
/*  Queues                                                                                       */
/*************************************************************************************************/

struct DvzQueueCaps
{
    uint32_t family_count;
    VkQueueFlags flags[DVZ_MAX_QUEUE_FAMILIES];
    uint32_t queue_count[DVZ_MAX_QUEUE_FAMILIES];
};



struct DvzQueue
{
    uint32_t family_idx;
    uint32_t queue_idx;
    VkQueue handle;
    VkQueueFlags flags;
    bool is_main; // whether this queue is the main one
    bool is_set;  // whether this queue exists
};



struct DvzQueues
{
    uint32_t queue_count;
    DvzQueue queues[DVZ_QUEUE_COUNT]; // for each role, a dedicated queue, or none.
};



/*************************************************************************************************/
/*  Memory                                                                                       */
/*************************************************************************************************/

struct DvzVma
{
    DvzDevice* device;
    VmaAllocator vma;
    VkExternalMemoryHandleTypeFlagsKHR external;
};



struct DvzAllocation
{
    VmaMemoryUsage usage;
    VmaAllocationCreateFlags flags;
    VmaAllocationInfo info;
    VmaAllocation alloc;
    VkMemoryPropertyFlags memory_flags;
    VkDeviceSize alignment; // alignment required by Vulkan
    void* mmap;
};



/*************************************************************************************************/
/*  Bootstrap                                                                                    */
/*************************************************************************************************/

struct DvzDevice
{
    DvzObject obj;
    DvzGpu* gpu;

    DvzQueues queues;

    uint32_t req_extension_count;
    char* req_extensions[DVZ_MAX_REQ_EXTENSIONS];

    VkPhysicalDeviceFeatures2 features;
    VkPhysicalDeviceVulkan11Features features11;
    VkPhysicalDeviceVulkan12Features features12;
    VkPhysicalDeviceVulkan13Features features13;

    VkDevice vk_device;
    VkCommandPool cpools[DVZ_MAX_QUEUE_FAMILIES];
    VkDescriptorPool dpool;
};



struct DvzGpu
{
    DvzInstance* instance;

    VkPhysicalDevice pdevice;

    VkPhysicalDeviceProperties2 props;
    VkPhysicalDeviceVulkan11Properties props11;
    VkPhysicalDeviceVulkan12Properties props12;
    VkPhysicalDeviceVulkan13Properties props13;

    VkPhysicalDeviceMemoryProperties2 memprops;

    VkPhysicalDeviceFeatures2 features;
    VkPhysicalDeviceVulkan11Features features11;
    VkPhysicalDeviceVulkan12Features features12;
    VkPhysicalDeviceVulkan13Features features13;

    // All supported device extensions.
    uint32_t extension_count;
    char** extensions;

    DvzQueueCaps queue_caps;
};



struct DvzInstance
{
    DvzObject obj;

    VkInstance vk_instance;
    uint32_t vk_version;

    // Instance creation structures.
    bool flags;
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

    // GPUs
    uint32_t gpu_count;
    DvzGpu gpus[DVZ_MAX_GPUS];
};



struct DvzBootstrap
{
    int flags;
    DvzInstance instance;
    DvzGpu* gpu;
    DvzDevice device;
    DvzVma allocator;
};
