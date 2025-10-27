/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Compute                                                                                      */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "datoviz/common/macros.h"
#include "vulkan/vulkan_core.h"
#include <stdint.h>
#include <volk.h>



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzDevice DvzDevice;

typedef struct DvzCompute DvzCompute;



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

EXTERN_C_ON



/**
 * Initialize a compute pipeline.
 *
 * @param device the device
 * @param[out] compute the compute pipeline
 */
DVZ_EXPORT void dvz_compute(DvzDevice* device, DvzCompute* compute);



/**
 * Set the shader module.
 *
 * @param compute the compute pipeline
 * @param shader the shader
 */
DVZ_EXPORT void dvz_compute_shader(DvzCompute* compute, VkShaderModule module);



/**
 * Set the pipeline layout.
 *
 * @param compute the compute pipeline
 * @param layout the pipeline layout
 */
DVZ_EXPORT void dvz_compute_layout(DvzCompute* compute, VkPipelineLayout layout);



/**
 * Create a compute pipeline after it has been set up.
 *
 * @param compute the compute pipeline
 * @returns the creation result code
 */
DVZ_EXPORT int dvz_compute_create(DvzCompute* compute);



/**
 * Destroy a compute pipeline.
 *
 * @param compute the compute pipeline
 */
DVZ_EXPORT void dvz_compute_destroy(DvzCompute* compute);



EXTERN_C_OFF
