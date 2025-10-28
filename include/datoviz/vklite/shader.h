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

#include "datoviz/common/macros.h"
#include "datoviz/math/types.h"
#include "vulkan/vulkan_core.h"
#include <stdint.h>
#include <volk.h>



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzDevice DvzDevice;

typedef struct DvzShader DvzShader;



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

EXTERN_C_ON



/**
 * Create a shader module.
 *
 * @param device the device
 * @param size the size of the buffer with the SPIR-V code, in bytes
 * @param buffer the buffer with the SPIR-V bytecode
 * @param[out] shader the shader module
 * @returns the Vulkan creation result code
 */
DVZ_EXPORT int
dvz_shader(DvzDevice* device, DvzSize size, const uint32_t* buffer, DvzShader* shader);



/**
 * Return the shader Vulkan handle.
 *
 * @param shader the shader
 * @returns the shader module handle
 */
DVZ_EXPORT VkShaderModule dvz_shader_module(DvzShader* shader);



/**
 * Destroy a shader module.
 *
 * @param shader the shader module
 */
DVZ_EXPORT void dvz_shader_destroy(DvzShader* shader);



EXTERN_C_OFF
