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

#include "datoviz/vklite/compute.h"
#include "../src/vk/macros.h"
#include "_assertions.h"
#include "_compat.h"
#include "_log.h"
#include "datoviz/common/obj.h"
#include "datoviz/math/types.h"
#include "datoviz/vk/device.h"
#include "datoviz/vk/queues.h"
#include <volk.h>



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



void dvz_compute_spec(
    DvzCompute* compute, uint32_t index, DvzSize offset, DvzSize size, void* data)
{
    ANN(compute);
    ANN(data);

    if (index >= DVZ_MAX_SPEC_CONST)
    {
        log_error(
            "there can be no more than %d specialization constants per pipeline",
            DVZ_MAX_SPEC_CONST);
        return;
    }

    if (offset + size >= DVZ_MAX_SPEC_CONST_SIZE)
    {
        log_error(
            "the specialization constant data buffer can be no more than %s",
            dvz_pretty_size(DVZ_MAX_SPEC_CONST_SIZE));
        return;
    }

    compute->spec_entries[index].constantID = index;
    compute->spec_entries[index].offset = offset;
    compute->spec_entries[index].size = size;

    compute->spec_info.mapEntryCount = MAX(compute->spec_info.mapEntryCount, index + 1);
    compute->spec_info.dataSize = MAX(compute->spec_info.dataSize, offset + size);
    ASSERT(compute->spec_info.dataSize <= DVZ_MAX_SPEC_CONST_SIZE);

    // Copy the value in the specialization constant data buffer.
    dvz_memcpy(&compute->spec_data[offset], size, data, size);
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

    VkPipelineShaderStageCreateInfo stage = {0};
    stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    stage.pName = "main";
    stage.module = compute->shader;
    compute->spec_info.pMapEntries = compute->spec_entries;
    compute->spec_info.pData = compute->spec_data;
    stage.pSpecializationInfo = &compute->spec_info;

    VkComputePipelineCreateInfo pipelineInfo = {0};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipelineInfo.stage = stage;
    pipelineInfo.layout = compute->layout;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

    VK_RETURN_RESULT(vkCreateComputePipelines(
        dvz_device_handle(compute->device), VK_NULL_HANDLE, 1, &pipelineInfo, NULL,
        &compute->vk_pipeline));
    if (out == 0)
    {
        dvz_obj_created(&compute->obj);
        log_trace("compute created");
    }

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

    if (compute->vk_pipeline != VK_NULL_HANDLE)
    {
        log_trace("destroying compute...");
        vkDestroyPipeline(dvz_device_handle(compute->device), compute->vk_pipeline, NULL);
        compute->vk_pipeline = VK_NULL_HANDLE;
        log_trace("compute destroyed");
    }

    dvz_obj_destroyed(&compute->obj);
}
