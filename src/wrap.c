/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Host                                                                                         */
/*************************************************************************************************/


#include "wrap.h"
#include "canvas.h"
#include "common.h"
#include "host.h"
#include "vklite.h"
#include "vkutils.h"



/*************************************************************************************************/
/*  Wrappers                                                                                     */
/*************************************************************************************************/

DvzHost* dvz_host_wrap(VkInstance instance)
{
    DvzHost* host = (DvzHost*)calloc(1, sizeof(DvzHost));
    ANN(host);
    dvz_obj_init(&host->obj);
    host->obj.type = DVZ_OBJECT_TYPE_HOST;
    // host->backend = DVZ_BACKEND_WRAP;
    host->instance = instance;
    host->gpus = dvz_container(DVZ_CONTAINER_DEFAULT_COUNT, sizeof(DvzGpu), DVZ_OBJECT_TYPE_GPU);
    return host;
}



DvzGpu* dvz_gpu_wrap(DvzHost* host, VkDevice device)
{
    ANN(host);
    uint32_t i = 0;
    DvzGpu* gpu = (DvzGpu*)dvz_container_alloc(&host->gpus);
    ANN(gpu);

    gpu->host = host;
    gpu->idx = i;
    gpu->device = device;

    dvz_obj_created(&gpu->obj);
    return gpu;
}



DvzCanvas* dvz_canvas_wrap(DvzGpu* gpu, DvzRenderpass* renderpass, DvzFramebuffers* framebuffers)
{
    ANN(gpu);
    ANN(renderpass);
    ANN(framebuffers);


    DvzCanvas* canvas = (DvzCanvas*)calloc(1, sizeof(DvzCanvas));
    canvas->obj.type = DVZ_OBJECT_TYPE_CANVAS;
    canvas->gpu = gpu;

    canvas->render.renderpass = renderpass;
    canvas->render.framebuffers = *framebuffers;
    dvz_obj_created(&canvas->obj);

    return canvas;
}



DvzCommands dvz_commands_wrap(DvzGpu* gpu, uint32_t img_count)
{
    ANN(gpu);
    DvzCommands commands = {0};
    commands.gpu = gpu;
    commands.count = img_count;
    return commands;
}



void dvz_commands_set(DvzCommands* cmds, uint32_t img_idx, VkCommandBuffer cmd)
{
    ANN(cmds);
    ASSERT(img_idx < DVZ_MAX_SWAPCHAIN_IMAGES);
    cmds->cmds[img_idx] = cmd;
}



DvzRenderpass dvz_renderpass_wrap(DvzGpu* gpu, VkRenderPass vk_renderpass)
{
    ANN(gpu);
    DvzRenderpass renderpass = {0};
    renderpass.gpu = gpu;
    renderpass.renderpass = vk_renderpass;
    dvz_obj_created(&renderpass.obj);
    return renderpass;
}



DvzFramebuffers dvz_framebuffers_wrap(DvzGpu* gpu, DvzRenderpass* renderpass, uint32_t img_count)
{
    ANN(gpu);

    DvzFramebuffers framebuffers = {0};
    framebuffers.gpu = gpu;
    framebuffers.renderpass = renderpass;
    framebuffers.framebuffer_count = img_count;
    dvz_obj_created(&framebuffers.obj);
    return framebuffers;
}



void dvz_framebuffers_set(
    DvzFramebuffers* framebuffers, uint32_t img_idx, VkFramebuffer framebuffer)
{
    ANN(framebuffers);
    ASSERT(img_idx < DVZ_MAX_SWAPCHAIN_IMAGES);
    framebuffers->framebuffers[img_idx] = framebuffer;
}
