/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Shader                                                                                       */
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
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzDevice DvzDevice;

typedef struct DvzShader DvzShader;


/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

EXTERN_C_ON



/**
 * Allocate an empty shader wrapper.
 *
 * Heap-allocated wrappers follow the same lifecycle as stack-owned wrappers:
 * call dvz_shader() once per live wrapper, destroy before any recreate, and
 * free only if this wrapper came from dvz_shader_create_wrapper().
 *
 * @return allocated shader wrapper, or NULL on allocation failure
 */
DVZ_EXPORT DvzShader* dvz_shader_create_wrapper(void);



/**
 * Create a shader module.
 *
 * This function creates the wrapped Vulkan shader module exactly once per live
 * wrapper. Call dvz_shader_destroy() before attempting to create it again.
 *
 * @param device the device
 * @param size the size of the buffer with the SPIR-V code, in bytes
 * @param buffer the buffer with the SPIR-V bytecode
 * @param[out] shader the shader module
 * @return the Vulkan creation result code
 */
DVZ_EXPORT int
dvz_shader(DvzDevice* device, DvzSize size, const uint32_t* buffer, DvzShader* shader);



/**
 * Return the shader Vulkan handle.
 *
 * @param shader the shader
 * @returns the shader module handle
 */
DVZ_EXPORT VkShaderModule dvz_shader_handle(DvzShader* shader);



/**
 * Destroy a shader module.
 *
 * This releases the wrapped Vulkan shader module and returns the wrapper to a
 * reusable initialized state.
 *
 * @param shader the shader module
 */
DVZ_EXPORT void dvz_shader_destroy(DvzShader* shader);



/**
 * Free a shader wrapper allocated by dvz_shader_create_wrapper().
 *
 * @param shader shader wrapper to free
 */
DVZ_EXPORT void dvz_shader_free(DvzShader* shader);



EXTERN_C_OFF
