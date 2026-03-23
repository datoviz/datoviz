/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Testing commands                                                                             */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdint.h>

#include "test_vk.h"
#include "_assertions.h"
#include "datoviz/vk/device.h"
#include "datoviz/vk/gpu_ctx.h"
#include "datoviz/vk/queues.h"
#include "datoviz/vklite/commands.h"
#include "datoviz/vklite/sync.h"
#include "test_vklite.h"
#include "testing.h"



/*************************************************************************************************/
/*  vklite tests                                                                                 */
/*************************************************************************************************/

/**
 * Create a GPU context suitable for command-submission tests.
 *
 * @return allocated GPU context, or NULL on failure
 */
static DvzGpuCtx* _commands_ctx(void)
{
    DvzGpuCtxConfig cfg = dvz_gpu_ctx_config();
    VkPhysicalDeviceVulkan13Features features13 = {0};
    features13.synchronization2 = true;
    dvz_gpu_ctx_config_features13(&cfg, &features13);
    return dvz_gpu_ctx(&cfg);
}

int test_vklite_commands_1(TstSuite* suite, TstItem* tstitem)
{
    ANN(suite);
    ANN(tstitem);

    DvzGpuCtx* ctx = _commands_ctx();
    ANN(ctx);

    DvzDevice* device = dvz_gpu_ctx_device(ctx);
    ANN(device);

    DvzQueue* queue = dvz_device_queue(device, DVZ_QUEUE_MAIN);
    ANN(queue);

    DvzCommands cmds = {0};
    dvz_commands(device, queue, 3, &cmds);
    dvz_cmd_begin(&cmds);
    dvz_cmd_end(&cmds);
    dvz_cmd_reset(&cmds);
    dvz_cmd_free(&cmds);

    uint32_t err_count = dvz_gpu_ctx_error_count(ctx);
    dvz_gpu_ctx_destroy(ctx);

    return err_count > 0;
}



int test_vklite_commands_repeat_submit(TstSuite* suite, TstItem* tstitem)
{
    ANN(suite);
    ANN(tstitem);

    DvzGpuCtx* ctx = _commands_ctx();
    ANN(ctx);

    DvzDevice* device = dvz_gpu_ctx_device(ctx);
    DvzQueue* queue = dvz_device_queue(device, DVZ_QUEUE_MAIN);
    ANN(device);
    ANN(queue);

    DvzCommands cmds = {0};
    dvz_commands(device, queue, 1, &cmds);
    dvz_cmd_begin(&cmds);
    dvz_cmd_end(&cmds);

    dvz_cmd_submit(&cmds);
    dvz_cmd_submit(&cmds);

    uint32_t err_count = dvz_gpu_ctx_error_count(ctx);
    dvz_commands_destroy(&cmds);
    dvz_gpu_ctx_destroy(ctx);

    return err_count > 0;
}



int test_vklite_commands_destroy_idempotent(TstSuite* suite, TstItem* tstitem)
{
    ANN(suite);
    ANN(tstitem);

    DvzGpuCtx* ctx = _commands_ctx();
    ANN(ctx);

    DvzDevice* device = dvz_gpu_ctx_device(ctx);
    DvzQueue* queue = dvz_device_queue(device, DVZ_QUEUE_MAIN);
    ANN(device);
    ANN(queue);

    DvzCommands cmds = {0};
    dvz_commands(device, queue, 1, &cmds);
    dvz_cmd_begin(&cmds);
    dvz_cmd_end(&cmds);

    dvz_commands_destroy(&cmds);
    dvz_commands_destroy(&cmds);
    dvz_commands_destroy(NULL);
    AT(cmds.count == 0);
    AT(dvz_commands_handle(&cmds) == VK_NULL_HANDLE);

    DvzFence fence = {0};
    dvz_fence(device, false, &fence);
    dvz_fence_destroy(&fence);
    dvz_fence_destroy(&fence);

    DvzSemaphore semaphore = {0};
    dvz_semaphore(device, &semaphore);
    dvz_semaphore_destroy(&semaphore);
    dvz_semaphore_destroy(&semaphore);

    uint32_t err_count = dvz_gpu_ctx_error_count(ctx);
    dvz_gpu_ctx_destroy(ctx);

    return err_count > 0;
}



int test_vklite_barriers_reset(TstSuite* suite, TstItem* tstitem)
{
    ANN(suite);
    ANN(tstitem);

    DvzBarriers barriers = {0};
    dvz_barriers(&barriers);

    for (uint32_t i = 0; i < DVZ_MAX_BARRIERS; i++)
    {
        DvzBarrierImage* bimg = dvz_barriers_image(&barriers, (VkImage)(uintptr_t)(i + 1));
        AT(bimg != NULL);
    }
    AT(barriers.info.imageMemoryBarrierCount == DVZ_MAX_BARRIERS);
    AT_EXPECTED_ERROR_STRICT(
        suite, (dvz_barriers_image(&barriers, (VkImage)(uintptr_t)0xFFFF) == NULL));

    dvz_barriers(&barriers);
    AT(barriers.info.imageMemoryBarrierCount == 0);
    AT(barriers.info.bufferMemoryBarrierCount == 0);
    AT(barriers.info.memoryBarrierCount == 0);

    DvzBarrierImage* bimg = dvz_barriers_image(&barriers, (VkImage)(uintptr_t)0x1234);
    AT(bimg != NULL);
    AT(barriers.info.imageMemoryBarrierCount == 1);
    AT(bimg->subresourceRange.aspectMask == VK_IMAGE_ASPECT_COLOR_BIT);
    AT(bimg->subresourceRange.levelCount == 1);
    AT(bimg->subresourceRange.layerCount == 1);

    return 0;
}
