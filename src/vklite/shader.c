/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Shader                                                                                       */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/


#include <stddef.h>
#include <volk.h>

#include "_vk_utils.h"
#include "_alloc.h"
#include "_assertions.h"
#include "_compat.h"
#include "_log.h"
#include "_shader.h"
#include "datoviz/vk/device.h"
#include "datoviz/vk/queues.h"



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Allocate an empty shader wrapper.
 *
 * @return allocated shader wrapper, or NULL on allocation failure
 */
DvzShader* dvz_shader_create_wrapper(void)
{
    DvzShader* shader = (DvzShader*)dvz_calloc(1, sizeof(DvzShader));
    ANN(shader);
    return shader;
}



/**
 * Create a shader module.
 *
 * @param device the device
 * @param size the size of the buffer with the SPIR-V code, in bytes
 * @param buffer the buffer with the SPIR-V bytecode
 * @param[out] shader the shader module
 * @return the Vulkan creation result code
 */
int dvz_shader(DvzDevice* device, DvzSize size, const uint32_t* buffer, DvzShader* shader)
{
    ANN(device);
    ANN(buffer);
    ANN(shader);
    ASSERT(size > 0);
    if (dvz_obj_is_created(&shader->obj))
    {
        log_error("cannot create a shader twice without destroying it first");
        return 1;
    }

    VkDevice vkd = dvz_device_handle(device);
    ANNVK(vkd);

    shader->device = device;
    shader->size = size;
    shader->buffer = (uint32_t*)dvz_calloc(size, 1);
    dvz_memcpy(shader->buffer, size, buffer, size);

    VkShaderModuleCreateInfo info = {0};
    info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    info.codeSize = (size_t)size;
    info.pCode = buffer;
    log_trace("creating shader module...");
    VK_RETURN_RESULT(vkCreateShaderModule(vkd, &info, NULL, &shader->vk_shader));
    if (out == 0)
    {
        dvz_obj_created(&shader->obj);
        log_trace("shader module created");
    }

    return out;
}



/**
 * Return the shader Vulkan handle.
 *
 * @param shader the shader
 * @return the shader module handle
 */
VkShaderModule dvz_shader_handle(DvzShader* shader)
{
    ANN(shader);
    return shader->vk_shader;
}



/**
 * Destroy a shader module.
 *
 * @param shader the shader module
 */
void dvz_shader_destroy(DvzShader* shader)
{
    ANN(shader);
    if (!dvz_obj_is_created(&shader->obj))
        return;

    ANN(shader->buffer);
    ANN(shader->device);
    VkDevice vkd = dvz_device_handle(shader->device);
    ANNVK(vkd);

    log_trace("destroying shader module...");
    vkDestroyShaderModule(vkd, shader->vk_shader, NULL);
    log_trace("shader module destroyed");

    shader->vk_shader = VK_NULL_HANDLE;
    dvz_free(shader->buffer);
    shader->buffer = NULL;
    dvz_obj_destroyed(&shader->obj);
}



/**
 * Free a shader wrapper allocated by dvz_shader_create_wrapper().
 *
 * @param shader shader wrapper to free
 */
void dvz_shader_free(DvzShader* shader)
{
    if (shader == NULL)
    {
        return;
    }
    dvz_free(shader);
}
