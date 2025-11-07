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

#include <stdint.h>
#include <vulkan/vulkan_core.h>

#include "datoviz/common/macros.h"
#include "datoviz/common/obj.h"
#include "datoviz/math/types.h"
#include "datoviz/vk/device.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

// Arbitrarily limit the spec constant data buffer size which simplifies the implementation.
#define DVZ_MAX_SPEC_CONST_SIZE 128
#define DVZ_MAX_SPEC_CONST      8



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzDevice DvzDevice;
typedef struct DvzCommands DvzCommands;

typedef struct DvzCompute DvzCompute;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzCompute
{
    DvzObject obj;
    DvzDevice* device;

    VkShaderModule shader;
    VkPipelineLayout layout;

    VkSpecializationMapEntry spec_entries[DVZ_MAX_SPEC_CONST];
    VkSpecializationInfo spec_info;
    unsigned char spec_data[DVZ_MAX_SPEC_CONST_SIZE]; // specialization constant data buffer

    VkPipeline vk_pipeline;
};



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
 * Set a specialization constant.
 *
 * @param compute the compute pipeline
 * @param index the specialization constant index in the shader
 * @param offset the offset, in bytes, of that constant, without the specialization constant data
 * @param size the size of the specialization constant value
 * @param data the value of the constant
 */
DVZ_EXPORT void
dvz_compute_spec(DvzCompute* compute, uint32_t index, DvzSize offset, DvzSize size, void* data);



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



/**
 * Bind a compute pipeline.
 *
 * @param cmds the set of command buffers to record
 * @param idx the index of the command buffer to record
 * @param compute the compute pipeline
 */
DVZ_EXPORT void dvz_cmd_bind_compute(DvzCommands* cmds, uint32_t idx, DvzCompute* compute);



/**
 * Dispatch a compute task.
 *
 * @param cmds the set of command buffers to record
 * @param idx the index of the command buffer to record
 * @param nx the number of local workgroups to dispatch in the X dimension
 * @param ny the number of local workgroups to dispatch in the Y dimension
 * @param nz the number of local workgroups to dispatch in the Z dimension
 */
DVZ_EXPORT void
dvz_cmd_dispatch(DvzCommands* cmds, uint32_t idx, uint32_t nx, uint32_t ny, uint32_t nz);



EXTERN_C_OFF
