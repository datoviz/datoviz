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
#include <vulkan/vulkan_core.h>

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
 * @return allocated shader wrapper, or NULL on allocation failure
 */
DVZ_EXPORT DvzShader* dvz_shader_create_wrapper(void);



/**
 * Create a shader module.
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
