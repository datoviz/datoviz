/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Compute                                                                                      */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdint.h>

#include "../src/vk/macros.h"
#include "_assertions.h"
#include "_log.h"
#include "datoviz/common/macros.h"
#include "datoviz/common/obj.h"
#include "datoviz/vk/device.h"
#include "datoviz/vk/queues.h"
#include "datoviz/vklite/compute.h"
#include "types.h"
#include "vulkan/vulkan_core.h"



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

void dvz_compute(DvzDevice* device, DvzCompute* compute)
{
    ANN(device);
    ANN(compute);
    compute->device = device;
    dvz_obj_init(&compute->obj);
}



void dvz_compute_shader(DvzCompute* compute, VkShaderModule shader)
{
    ANN(compute);
    compute->shader = shader;
}



void dvz_compute_layout(DvzCompute* compute, VkPipelineLayout layout)
{
    ANN(compute);
    compute->layout = layout;
}



int dvz_compute_create(DvzCompute* compute)
{
    ANN(compute);
    ANN(compute->device);

    if (compute->layout == VK_NULL_HANDLE)
    {
        log_error("cannot create compute pipeline without layout");
        return 1;
    }
    if (compute->shader == VK_NULL_HANDLE)
    {
        log_error("cannot create compute pipeline without shader");
        return 2;
    }

    VkComputePipelineCreateInfo pipelineInfo = {0};
    pipelineInfo.stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipelineInfo.stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    pipelineInfo.layout = compute->layout;
    pipelineInfo.stage.pName = "main";
    pipelineInfo.stage.module = compute->shader;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

    VK_RETURN_RESULT(vkCreateComputePipelines(
        dvz_device_handle(compute->device), VK_NULL_HANDLE, 1, &pipelineInfo, NULL,
        &compute->vk_pipeline));
    dvz_obj_created(&compute->obj);
    log_trace("compute created");

    return out;
}



void dvz_compute_destroy(DvzCompute* compute)
{
    ANN(compute);
    ANN(compute->device);

    if (!dvz_obj_is_created(&compute->obj))
    {
        log_trace("skip destruction of already-destroyed compute");
        return;
    }
    log_trace("destroy compute");

    if (compute->vk_pipeline != VK_NULL_HANDLE)
    {
        vkDestroyPipeline(dvz_device_handle(compute->device), compute->vk_pipeline, NULL);
        compute->vk_pipeline = VK_NULL_HANDLE;
    }

    dvz_obj_destroyed(&compute->obj);
}
