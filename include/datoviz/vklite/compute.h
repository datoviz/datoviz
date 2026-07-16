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
#include "datoviz/vk/vulkan.h"

#include "datoviz/common/macros.h"
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
/*  Functions                                                                                    */
/*************************************************************************************************/

EXTERN_C_ON



/**
 * Allocate an empty compute wrapper.
 *
 * Heap-allocated wrappers follow the same lifecycle as stack-owned wrappers:
 * initialize with dvz_compute(), configure, call dvz_compute_create() once,
 * then destroy before any recreate and free only if this wrapper came from
 * dvz_compute_create_wrapper().
 *
 * @return allocated compute wrapper, or NULL on allocation failure
 */
DVZ_EXPORT DvzCompute* dvz_compute_create_wrapper(void);



/**
 * Initialize a compute pipeline.
 *
 * This prepares the wrapper for configuration. Call dvz_compute_create() once
 * after setting the shader module and pipeline layout. Recreating a live
 * compute pipeline requires dvz_compute_destroy() first.
 *
 * @param device the device
 * @param[out] compute the compute pipeline
 */
DVZ_EXPORT void dvz_compute(DvzDevice* device, DvzCompute* compute);



/**
 * Set the shader module.
 *
 * @param compute the compute pipeline
 * @param module borrowed compute shader module; it must remain live through pipeline creation
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
 * This function creates the wrapped Vulkan pipeline exactly once per live
 * wrapper. Call dvz_compute_destroy() before attempting to create it again.
 *
 * @param compute the compute pipeline
 * @return the creation result code
 */
DVZ_EXPORT int dvz_compute_create(DvzCompute* compute);



/**
 * Return the Vulkan pipeline handle owned by a compute wrapper.
 *
 * @param compute the compute pipeline
 * @return borrowed Vulkan pipeline handle, or `VK_NULL_HANDLE` when not created
 */
DVZ_EXPORT VkPipeline dvz_compute_handle(DvzCompute* compute);



/**
 * Return the pipeline layout bound to a compute wrapper.
 *
 * @param compute the compute pipeline
 * @return borrowed pipeline-layout handle, or `VK_NULL_HANDLE` when unset
 */
DVZ_EXPORT VkPipelineLayout dvz_compute_layout_handle(DvzCompute* compute);



/**
 * Destroy a compute pipeline.
 *
 * This releases the wrapped Vulkan pipeline and returns the wrapper to a
 * reusable initialized state.
 *
 * @param compute the compute pipeline
 */
DVZ_EXPORT void dvz_compute_destroy(DvzCompute* compute);



/**
 * Free a compute wrapper allocated by dvz_compute_create_wrapper().
 *
 * @param compute compute wrapper to free
 */
DVZ_EXPORT void dvz_compute_free(DvzCompute* compute);



/**
 * Bind a compute pipeline.
 *
 * @param cmds the set of command buffers to record
 * @param compute the compute pipeline
 */
DVZ_EXPORT void dvz_cmd_bind_compute(DvzCommands* cmds, DvzCompute* compute);



/**
 * Dispatch a compute task.
 *
 * @param cmds the set of command buffers to record
 * @param nx the number of local workgroups to dispatch in the X dimension
 * @param ny the number of local workgroups to dispatch in the Y dimension
 * @param nz the number of local workgroups to dispatch in the Z dimension
 */
DVZ_EXPORT void dvz_cmd_dispatch(DvzCommands* cmds, uint32_t nx, uint32_t ny, uint32_t nz);



EXTERN_C_OFF
