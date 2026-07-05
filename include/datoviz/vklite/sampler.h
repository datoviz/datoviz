/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Sampler                                                                                      */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdint.h>
#include "datoviz/vk/vulkan.h"

#include "datoviz/common/macros.h"



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzDevice DvzDevice;

typedef struct DvzSampler DvzSampler;



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/

// Texture axis.
typedef enum
{
    DVZ_SAMPLER_AXIS_U,
    DVZ_SAMPLER_AXIS_V,
    DVZ_SAMPLER_AXIS_W,
} DvzSamplerAxis;



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

EXTERN_C_ON



/**
 * Allocate an empty sampler wrapper.
 *
 * Heap-allocated wrappers follow the same lifecycle as stack-owned wrappers:
 * initialize with dvz_sampler(), configure, call dvz_sampler_create() once,
 * then destroy before any recreate and free only if this wrapper came from
 * dvz_sampler_create_wrapper().
 *
 * @return allocated sampler wrapper, or NULL on allocation failure
 */
DVZ_EXPORT DvzSampler* dvz_sampler_create_wrapper(void);



/**
 * Initialize a texture sampler.
 *
 * This prepares the wrapper for configuration. Call dvz_sampler_create() once
 * after setting filter and address-mode state. Recreating a live sampler
 * requires dvz_sampler_destroy() first.
 *
 * @param device the device
 * @param sampler the sampler object to create
 */
DVZ_EXPORT void dvz_sampler(DvzDevice* device, DvzSampler* sampler);



/**
 * Set the sampler min filter.
 *
 * @param sampler the sampler
 * @param filter the filter
 */
DVZ_EXPORT void dvz_sampler_min_filter(DvzSampler* sampler, VkFilter filter);



/**
 * Set the sampler mag filter.
 *
 * @param sampler the sampler
 * @param filter the filter
 */
DVZ_EXPORT void dvz_sampler_mag_filter(DvzSampler* sampler, VkFilter filter);



/**
 * Set the sampler address mode
 *
 * @param sampler the sampler
 * @param axis the sampler axis
 * @param address_mode the address mode
 */
DVZ_EXPORT void dvz_sampler_address_mode(
    DvzSampler* sampler, DvzSamplerAxis axis, VkSamplerAddressMode address_mode);



/**
 * Set the anisotropy.
 *
 * @param sampler the sampler
 * @param anisotropy anisotropy
 */
DVZ_EXPORT void dvz_sampler_anisotropy(DvzSampler* sampler, float anisotropy);



/**
 * Create the sampler after it has been set up.
 *
 * This function creates the wrapped Vulkan sampler exactly once per live
 * wrapper. Call dvz_sampler_destroy() before attempting to create it again.
 *
 * @param sampler the sampler
 * @returns the creation result code
 */
DVZ_EXPORT int dvz_sampler_create(DvzSampler* sampler);



/**
 * Destroy a sampler.
 *
 * This releases the wrapped Vulkan sampler and returns the wrapper to a
 * reusable initialized state.
 *
 * @param sampler the sampler
 */
DVZ_EXPORT void dvz_sampler_destroy(DvzSampler* sampler);



/**
 * Return the Vulkan sampler handle.
 *
 * @param sampler sampler wrapper
 * @return wrapped Vulkan sampler handle
 */
DVZ_EXPORT VkSampler dvz_sampler_handle(DvzSampler* sampler);



/**
 * Free a sampler wrapper allocated by dvz_sampler_create_wrapper().
 *
 * @param sampler sampler wrapper to free
 */
DVZ_EXPORT void dvz_sampler_free(DvzSampler* sampler);



EXTERN_C_OFF
