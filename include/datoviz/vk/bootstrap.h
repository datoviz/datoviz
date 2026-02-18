/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/**
 * This module is mostly used to simplify test implementations, it is not meant for production
 * code.
 *
 */

/*************************************************************************************************/
/*  Bootstrap                                                                                    */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdint.h>
#include <stdbool.h>

#include "datoviz/common/macros.h"
#include "datoviz/vk/bootstrap.h"
#include "datoviz/vk/device.h"
#include "datoviz/vk/gpu.h"
#include "datoviz/vk/instance.h"
#include "datoviz/vk/memory.h"



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/

typedef enum
{
    DVZ_BOOTSTRAP_MANUAL_CREATE_INSTANCE = 0x01,
    DVZ_BOOTSTRAP_MANUAL_CREATE_DEVICE = 0x02,
    DVZ_BOOTSTRAP_MANUAL_CREATE_ALLOCATOR = 0x05,
} DvzBootstrapFlags;



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzBootstrap DvzBootstrap;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzBootstrap
{
    int flags;
    uint32_t validation_error_count;
    uint32_t gpu_index;
    bool owns_instance;
    bool owns_device;
    DvzInstance* instance;
    DvzDevice* device;
    DvzVma allocator;
};



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

EXTERN_C_ON



/**
 * Create a bootstrap, wrapping instance, GPU, device, allocator.
 *
 * @param bootstrap the bootstrap object
 * @param flags the creation flags
 *
 * @note `bootstrap->owns_instance` and `bootstrap->owns_device` indicate whether destroy will
 * reclaim these handles.
 */
DVZ_EXPORT void dvz_bootstrap(DvzBootstrap* bootstrap, int flags);



/**
 * Return the bootstrap's instance.
 *
 * @param bootstrap the bootstrap
 * @returns the instance
 */
DVZ_EXPORT DvzInstance* dvz_bootstrap_instance(DvzBootstrap* bootstrap);



/**
 * Attach an externally-created instance to a bootstrap.
 *
 * @param bootstrap the bootstrap
 * @param instance externally-created instance (or NULL to clear)
 * @param take_ownership whether bootstrap destroy should reclaim the instance
 * @returns whether the assignment was accepted
 */
DVZ_EXPORT bool
dvz_bootstrap_set_instance(DvzBootstrap* bootstrap, DvzInstance* instance, bool take_ownership);



/**
 * Return the bootstrap selected GPU index.
 *
 * @param bootstrap the bootstrap
 * @returns the GPU index in the instance, or UINT32_MAX if unavailable
 */
DVZ_EXPORT uint32_t dvz_bootstrap_gpu_index(DvzBootstrap* bootstrap);



/**
 * Return the bootstrap selected GPU descriptor.
 *
 * @param bootstrap the bootstrap
 * @param[out] out_info destination GPU descriptor
 * @returns whether the descriptor could be retrieved
 */
DVZ_EXPORT bool dvz_bootstrap_gpu_info(DvzBootstrap* bootstrap, DvzGpuInfo* out_info);



/**
 * Return the bootstrap's device.
 *
 * @param bootstrap the bootstrap
 * @returns the device
 */
DVZ_EXPORT DvzDevice* dvz_bootstrap_device(DvzBootstrap* bootstrap);



/**
 * Attach an externally-created device to a bootstrap.
 *
 * @param bootstrap the bootstrap
 * @param device externally-created device (or NULL to clear)
 * @param take_ownership whether bootstrap destroy should reclaim the device
 * @returns whether the assignment was accepted
 */
DVZ_EXPORT bool
dvz_bootstrap_set_device(DvzBootstrap* bootstrap, DvzDevice* device, bool take_ownership);



/**
 * Return the bootstrap's allocator.
 *
 * @param bootstrap the bootstrap
 * @returns the allocator
 */
DVZ_EXPORT DvzVma* dvz_bootstrap_allocator(DvzBootstrap* bootstrap);



/**
 * Create or refresh the bootstrap allocator from the attached device.
 *
 * @param bootstrap the bootstrap
 * @param export_handle_type external memory export flags
 * @returns Vulkan-style success code (0 on success)
 */
DVZ_EXPORT int dvz_bootstrap_create_allocator(
    DvzBootstrap* bootstrap, VkExternalMemoryHandleTypeFlagsKHR export_handle_type);



/**
 * Return the bootstrap validation error count.
 *
 * @param bootstrap the bootstrap
 * @returns the validation error count
 */
DVZ_EXPORT uint32_t dvz_bootstrap_error_count(DvzBootstrap* bootstrap);



/**
 * Destroy the instance, device, allocator.
 *
 * @param bootstrap the bootstrap
 *
 * @note Only resources owned by the bootstrap are destroyed (`owns_instance`/`owns_device`).
 */
DVZ_EXPORT void dvz_bootstrap_destroy(DvzBootstrap* bootstrap);



EXTERN_C_OFF
