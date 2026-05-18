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

int test_vklite_commands_1(TstContext* suite, const TstCase* tstitem)
{
    ANN(suite);
    ANN(tstitem);

    DvzGpuCtx* ctx = _commands_ctx();
    ANN(ctx);

    DvzDevice* device = dvz_gpu_ctx_device(ctx);
    ANN(device);

    DvzQueue* queue = dvz_device_queue(device, DVZ_QUEUE_MAIN);
    ANN(queue);

    DvzCommands* cmds = dvz_commands_create_wrapper();
    ANN(cmds);
    dvz_commands(device, queue, 3, cmds);
    dvz_cmd_begin(cmds);
    dvz_cmd_end(cmds);
    dvz_cmd_reset(cmds);
    dvz_cmd_release(cmds);
    dvz_commands_free(cmds);

    uint32_t err_count = dvz_gpu_ctx_error_count(ctx);
    dvz_gpu_ctx_destroy(ctx);

    return err_count > 0;
}



int test_vklite_commands_repeat_submit(TstContext* suite, const TstCase* tstitem)
{
    ANN(suite);
    ANN(tstitem);

    DvzGpuCtx* ctx = _commands_ctx();
    ANN(ctx);

    DvzDevice* device = dvz_gpu_ctx_device(ctx);
    DvzQueue* queue = dvz_device_queue(device, DVZ_QUEUE_MAIN);
    ANN(device);
    ANN(queue);

    DvzCommands* cmds = dvz_commands_create_wrapper();
    ANN(cmds);
    dvz_commands(device, queue, 1, cmds);
    dvz_cmd_begin(cmds);
    dvz_cmd_end(cmds);

    dvz_cmd_submit(cmds);
    dvz_cmd_submit(cmds);

    uint32_t err_count = dvz_gpu_ctx_error_count(ctx);
    dvz_commands_destroy(cmds);
    dvz_commands_free(cmds);
    dvz_gpu_ctx_destroy(ctx);

    return err_count > 0;
}



int test_vklite_commands_destroy_idempotent(TstContext* suite, const TstCase* tstitem)
{
    ANN(suite);
    ANN(tstitem);

    DvzGpuCtx* ctx = _commands_ctx();
    ANN(ctx);

    DvzDevice* device = dvz_gpu_ctx_device(ctx);
    DvzQueue* queue = dvz_device_queue(device, DVZ_QUEUE_MAIN);
    ANN(device);
    ANN(queue);

    DvzCommands* cmds = dvz_commands_create_wrapper();
    ANN(cmds);
    dvz_commands(device, queue, 1, cmds);
    dvz_cmd_begin(cmds);
    dvz_cmd_end(cmds);

    dvz_commands_destroy(cmds);
    dvz_commands_destroy(cmds);
    dvz_commands_destroy(NULL);
    AT(dvz_commands_count(cmds) == 0);
    AT(dvz_commands_handle(cmds) == VK_NULL_HANDLE);
    dvz_commands_free(cmds);

    DvzFence* fence = dvz_fence_create_wrapper();
    ANN(fence);
    dvz_fence(device, false, fence);
    dvz_fence_destroy(fence);
    dvz_fence_destroy(fence);
    dvz_fence_free(fence);

    DvzSemaphore* semaphore = dvz_semaphore_create_wrapper();
    ANN(semaphore);
    dvz_semaphore(device, semaphore);
    dvz_semaphore_destroy(semaphore);
    dvz_semaphore_destroy(semaphore);
    dvz_semaphore_free(semaphore);

    uint32_t err_count = dvz_gpu_ctx_error_count(ctx);
    dvz_gpu_ctx_destroy(ctx);

    return err_count > 0;
}



int test_vklite_commands_destroy_without_recording(TstContext* suite, const TstCase* tstitem)
{
    ANN(suite);
    ANN(tstitem);

    DvzGpuCtx* ctx = _commands_ctx();
    ANN(ctx);

    DvzDevice* device = dvz_gpu_ctx_device(ctx);
    DvzQueue* queue = dvz_device_queue(device, DVZ_QUEUE_MAIN);
    ANN(device);
    ANN(queue);

    DvzCommands* cmds = dvz_commands_create_wrapper();
    ANN(cmds);
    dvz_commands(device, queue, 1, cmds);
    AT(dvz_commands_count(cmds) == 1);

    dvz_commands_destroy(cmds);
    AT(dvz_commands_count(cmds) == 0);
    AT(dvz_commands_handle(cmds) == VK_NULL_HANDLE);

    dvz_commands(device, queue, 1, cmds);
    AT(dvz_commands_count(cmds) == 1);
    dvz_cmd_begin(cmds);
    dvz_cmd_end(cmds);
    dvz_commands_destroy(cmds);
    dvz_commands_free(cmds);

    uint32_t err_count = dvz_gpu_ctx_error_count(ctx);
    dvz_gpu_ctx_destroy(ctx);

    return err_count > 0;
}



int test_vklite_commands_borrowed_recording_rejects_lifecycle(TstContext* suite, const TstCase* tstitem)
{
    ANN(suite);
    ANN(tstitem);

    DvzGpuCtx* ctx = _commands_ctx();
    ANN(ctx);

    DvzDevice* device = dvz_gpu_ctx_device(ctx);
    ANN(device);

    DvzCommands* cmds = dvz_commands_create_wrapper();
    ANN(cmds);
    dvz_commands_wrap_borrowed_recording(
        device, (VkCommandBuffer)(uintptr_t)0x12345678, cmds);
    AT(dvz_commands_count(cmds) == 1);
    AT(dvz_commands_handle(cmds) != VK_NULL_HANDLE);

    AT_EXPECTED_ERROR_STRICT(suite, dvz_cmd_begin_result(cmds) != 0);
    AT_EXPECTED_ERROR_STRICT(suite, dvz_cmd_end_result(cmds) != 0);
    AT_EXPECTED_ERROR_STRICT(suite, dvz_cmd_submit_result(cmds) != 0);

    tst_expect_error_begin(suite);
    dvz_cmd_reset(cmds);
    AT(tst_expect_error_end(suite) == 0);
    AT(dvz_commands_count(cmds) == 1);

    tst_expect_error_begin(suite);
    dvz_commands_destroy(cmds);
    AT(tst_expect_error_end(suite) == 0);
    AT(dvz_commands_count(cmds) == 0);
    AT(dvz_commands_handle(cmds) == VK_NULL_HANDLE);
    dvz_commands_free(cmds);

    uint32_t err_count = dvz_gpu_ctx_error_count(ctx);
    dvz_gpu_ctx_destroy(ctx);

    return err_count > 0;
}



int test_vklite_barriers_reset(TstContext* suite, const TstCase* tstitem)
{
    ANN(suite);
    ANN(tstitem);

    DvzBarriers barriers = {0};
    dvz_barriers(&barriers);
    AT(dvz_barriers_capacity(&barriers) == DVZ_MAX_BARRIERS);
    AT(dvz_barriers_dependency_flags(&barriers) == 0);
    dvz_barriers_flags(&barriers, VK_DEPENDENCY_BY_REGION_BIT);
    AT(dvz_barriers_dependency_flags(&barriers) == VK_DEPENDENCY_BY_REGION_BIT);

    for (uint32_t i = 0; i < DVZ_MAX_BARRIERS; i++)
    {
        DvzBarrierImage* bimg = dvz_barriers_image(&barriers, (VkImage)(uintptr_t)(i + 1));
        AT(bimg != NULL);
    }
    AT(dvz_barriers_image_count(&barriers) == DVZ_MAX_BARRIERS);
    AT_EXPECTED_ERROR_STRICT(
        suite, (dvz_barriers_image(&barriers, (VkImage)(uintptr_t)0xFFFF) == NULL));

    dvz_barriers(&barriers);
    AT(dvz_barriers_dependency_flags(&barriers) == 0);
    AT(dvz_barriers_image_count(&barriers) == 0);
    AT(dvz_barriers_buffer_count(&barriers) == 0);
    AT(dvz_barriers_memory_count(&barriers) == 0);

    DvzBarrierImage* bimg = dvz_barriers_image(&barriers, (VkImage)(uintptr_t)0x1234);
    AT(bimg != NULL);
    AT(dvz_barriers_image_count(&barriers) == 1);
    AT(bimg->subresourceRange.aspectMask == VK_IMAGE_ASPECT_COLOR_BIT);
    AT(bimg->subresourceRange.levelCount == 1);
    AT(bimg->subresourceRange.layerCount == 1);

    return 0;
}



int test_vklite_submit_reset_reuse(TstContext* suite, const TstCase* tstitem)
{
    ANN(suite);
    ANN(tstitem);

    DvzGpuCtx* ctx = _commands_ctx();
    ANN(ctx);

    DvzDevice* device = dvz_gpu_ctx_device(ctx);
    DvzQueue* queue = dvz_device_queue(device, DVZ_QUEUE_MAIN);
    ANN(device);
    ANN(queue);

    DvzCommands* cmds = dvz_commands_create_wrapper();
    ANN(cmds);
    dvz_commands(device, queue, 1, cmds);
    dvz_cmd_begin(cmds);
    dvz_cmd_end(cmds);

    DvzSubmit* submit = dvz_submit_create_wrapper();
    ANN(submit);
    AT_EXPECTED_ERROR_STRICT(
        suite, (dvz_submit_send(submit, dvz_queue_handle(queue), VK_NULL_HANDLE) != VK_SUCCESS));

    dvz_submit(submit);
    AT(dvz_submit_is_empty(submit));
    AT(dvz_submit_wait_count(submit) == 0);
    AT(dvz_submit_signal_count(submit) == 0);
    AT(dvz_submit_command_count(submit) == 0);

    AT_EXPECTED_ERROR_STRICT(
        suite, (dvz_submit_send(submit, dvz_queue_handle(queue), VK_NULL_HANDLE) != VK_SUCCESS));

    dvz_submit_command(submit, dvz_commands_handle(cmds));
    AT(!dvz_submit_is_empty(submit));
    AT(dvz_submit_command_count(submit) == 1);

    DvzFence* fence = dvz_fence_create_wrapper();
    ANN(fence);
    dvz_fence(device, false, fence);
    AT(dvz_submit_send(submit, dvz_queue_handle(queue), dvz_fence_handle(fence)) == VK_SUCCESS);
    dvz_fence_wait(fence);

    dvz_submit(submit);
    AT(dvz_submit_is_empty(submit));
    AT(dvz_submit_command_count(submit) == 0);

    dvz_fence_reset(fence);
    dvz_submit_command(submit, dvz_commands_handle(cmds));
    AT(dvz_submit_send(submit, dvz_queue_handle(queue), dvz_fence_handle(fence)) == VK_SUCCESS);
    dvz_fence_wait(fence);
    dvz_fence_destroy(fence);
    dvz_fence_free(fence);
    dvz_submit_free(submit);

    uint32_t err_count = dvz_gpu_ctx_error_count(ctx);
    dvz_commands_destroy(cmds);
    dvz_commands_free(cmds);
    dvz_gpu_ctx_destroy(ctx);

    return err_count > 0;
}
