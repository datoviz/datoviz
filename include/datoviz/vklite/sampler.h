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

#include "datoviz/common/macros.h"
#include <stdint.h>
#include <volk.h>



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
 * Initialize a texture sampler.
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
 * @param sampler the sampler
 */
DVZ_EXPORT void dvz_sampler_create(DvzSampler* sampler);



/**
 * Destroy a sampler
 *
 * @param sampler the sampler
 */
DVZ_EXPORT void dvz_sampler_destroy(DvzSampler* sampler);



EXTERN_C_OFF
