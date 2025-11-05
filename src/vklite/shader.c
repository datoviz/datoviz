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

#include "datoviz/vklite/shader.h"
#include "../src/vk/macros.h"
#include "_alloc.h"
#include "_assertions.h"
#include "_compat.h"
#include "_log.h"
#include "datoviz/common/obj.h"
#include "datoviz/vk/device.h"
#include "datoviz/vk/queues.h"
#include <volk.h>



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

int dvz_shader(DvzDevice* device, DvzSize size, const uint32_t* buffer, DvzShader* shader)
{
    ANN(device);
    ANN(buffer);
    ANN(shader);
    ASSERT(size > 0);

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



VkShaderModule dvz_shader_handle(DvzShader* shader)
{
    ANN(shader);
    return shader->vk_shader;
}



void dvz_shader_destroy(DvzShader* shader)
{
    ANN(shader);
    ANN(shader->device);
    ANN(shader->buffer);

    VkDevice vkd = dvz_device_handle(shader->device);
    ANNVK(vkd);

    log_trace("destroying shader module...");
    vkDestroyShaderModule(vkd, shader->vk_shader, NULL);
    log_trace("shader module destroyed");

    dvz_free(shader->buffer);
    shader->buffer = NULL;
    dvz_obj_destroyed(&shader->obj);
}
