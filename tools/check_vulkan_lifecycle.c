/*
 * Copyright (c) 2026 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 *
 * Standalone Vulkan loader lifecycle diagnostic for sanitizer attribution. This tool deliberately
 * uses only Vulkan and libc: it must not link Datoviz, VMA, GLFW, or project allocation helpers.
 */

/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vulkan/vulkan.h>



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/



/**
 * Create and destroy a Vulkan instance and graphics device without submitting work.
 *
 * @param require_nvidia select vendor 0x10de when nonzero, otherwise use the first physical device
 * @param cycle one-based lifecycle number for diagnostic output
 * @return zero on success, one on Vulkan failure or unsupported enumeration bounds
 */
static int lifecycle(int require_nvidia, unsigned cycle)
{
    VkInstance instance = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;
    int result = 1;
    VkApplicationInfo application = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "standalone-vulkan-lsan",
        .apiVersion = VK_API_VERSION_1_3,
    };
    VkInstanceCreateInfo instance_info = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &application,
    };
    VkResult status = vkCreateInstance(&instance_info, NULL, &instance);
    if (status != VK_SUCCESS)
    {
        fprintf(stderr, "vkCreateInstance failed: %d\n", status);
        goto cleanup;
    }
    VkPhysicalDevice physical_devices[64] = {0};
    uint32_t physical_count = 64;
    status = vkEnumeratePhysicalDevices(instance, &physical_count, physical_devices);
    if (status != VK_SUCCESS || physical_count == 0)
    {
        fprintf(stderr, "vkEnumeratePhysicalDevices failed: %d count=%u\n", status, physical_count);
        goto cleanup;
    }
    VkPhysicalDevice physical = VK_NULL_HANDLE;
    VkPhysicalDeviceProperties properties = {0};
    for (uint32_t i = 0; i < physical_count; i++)
    {
        vkGetPhysicalDeviceProperties(physical_devices[i], &properties);
        if (!require_nvidia || properties.vendorID == 0x10de)
        {
            physical = physical_devices[i];
            break;
        }
    }
    if (physical == VK_NULL_HANDLE)
    {
        fprintf(stderr, "requested NVIDIA physical device not found\n");
        goto cleanup;
    }
    VkQueueFamilyProperties families[64] = {0};
    uint32_t family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physical, &family_count, NULL);
    if (family_count == 0 || family_count > 64)
    {
        fprintf(stderr, "unsupported queue-family count: %u\n", family_count);
        goto cleanup;
    }
    vkGetPhysicalDeviceQueueFamilyProperties(physical, &family_count, families);
    uint32_t family = UINT32_MAX;
    for (uint32_t i = 0; i < family_count; i++)
    {
        if (families[i].queueCount > 0 && (families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT))
        {
            family = i;
            break;
        }
    }
    if (family == UINT32_MAX)
    {
        fprintf(stderr, "graphics queue unavailable\n");
        goto cleanup;
    }
    float priority = 1.0f;
    VkDeviceQueueCreateInfo queue_info = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueFamilyIndex = family,
        .queueCount = 1,
        .pQueuePriorities = &priority,
    };
    VkDeviceCreateInfo device_info = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &queue_info,
    };
    status = vkCreateDevice(physical, &device_info, NULL, &device);
    if (status != VK_SUCCESS)
    {
        fprintf(stderr, "vkCreateDevice failed: %d\n", status);
        goto cleanup;
    }
    VkQueue queue = VK_NULL_HANDLE;
    vkGetDeviceQueue(device, family, 0, &queue);
    if (queue == VK_NULL_HANDLE)
    {
        fprintf(stderr, "vkGetDeviceQueue returned NULL\n");
        goto cleanup;
    }
    /* No commands submitted; no GPU work can outlive device destruction. */
    result = 0;
    fprintf(stderr, "cycle %u: %s vendor=0x%x driver=0x%x\n", cycle, properties.deviceName,
            properties.vendorID, properties.driverVersion);
cleanup:
    if (device != VK_NULL_HANDLE)
        vkDestroyDevice(device, NULL);
    if (instance != VK_NULL_HANDLE)
        vkDestroyInstance(instance, NULL);
    return result;
}



/**
 * Repeat the standalone Vulkan lifecycle for sanitizer leak and growth checks.
 *
 * @param argc argument count; expects a cycle count and a provider selector
 * @param argv arguments; cycle count is 1 to 5, selector "nvidia" requires NVIDIA
 * @return zero on success, two on invalid arguments, three on Vulkan lifecycle failure
 */
int main(int argc, char** argv)
{
    if (argc != 3)
        return 2;
    unsigned cycles = (unsigned)strtoul(argv[1], NULL, 10);
    if (cycles == 0 || cycles > 5)
        return 2;
    for (unsigned i = 0; i < cycles; i++)
        if (lifecycle(strcmp(argv[2], "nvidia") == 0, i + 1) != 0)
            return 3;
    return 0;
}
