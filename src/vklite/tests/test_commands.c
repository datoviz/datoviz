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
#include <string.h>
#include <volk.h>

#include "../../thread/thread_internal.h"
#include "../../thread/atomic_internal.h"
#include "../../vk/_device.h"
#include "_time_utils.h"
#include "test_vk.h"
#include "_assertions.h"
#include "datoviz/common/functions.h"
#include "datoviz/vk/device.h"
#include "datoviz/vk/gpu_ctx.h"
#include "datoviz/vk/queues.h"
#include "datoviz/vklite/commands.h"
#include "datoviz/vklite/buffers.h"
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
static DvzGpuCtx* _commands_ctx(const TstContext* suite)
{
    DvzGpuCtxConfig cfg = dvz_testing_gpu_ctx_config(suite);
    dvz_gpu_ctx_config_validation(&cfg, true);
    VkPhysicalDeviceVulkan12Features features12 = {0};
    features12.timelineSemaphore = true;
    dvz_gpu_ctx_config_features12(&cfg, &features12);
    VkPhysicalDeviceVulkan13Features features13 = {0};
    features13.synchronization2 = true;
    dvz_gpu_ctx_config_features13(&cfg, &features13);
    return dvz_gpu_ctx(&cfg);
}



typedef struct
{
    DvzSemaphore* semaphore;
    uint64_t value;
    int delay_ms;
} TimelineSignalData;



/**
 * Signal a timeline semaphore after a controlled delay.
 *
 * @param user_data timeline signal configuration
 * @return NULL
 */
static void* _delayed_timeline_signal(void* user_data)
{
    ANN(user_data);
    TimelineSignalData* data = (TimelineSignalData*)user_data;
    dvz_sleep(data->delay_ms);
    dvz_semaphore_signal(data->semaphore, data->value);
    return NULL;
}

int test_vklite_commands_1(TstContext* suite, const TstCase* tstitem)
{
    ANN(suite);
    ANN(tstitem);

    DvzGpuCtx* ctx = _commands_ctx(suite);
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

    DvzGpuCtx* ctx = _commands_ctx(suite);
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



typedef struct
{
    DvzSemaphore* semaphore;
    DvzAtomic finished;
    uint64_t timeout_ns;
} CommandsQueueGate;



/**
 * Release a blocked queue on timeout without hanging a failing submission test.
 *
 * @param user_data owned test gate, live until the thread is joined
 * @return NULL
 */
static void* _commands_queue_gate_release(void* user_data)
{
    CommandsQueueGate* gate = (CommandsQueueGate*)user_data;
    uint64_t start = dvz_time_monotonic_ns();
    while (!dvz_atomic_get(gate->finished))
    {
        if (dvz_time_monotonic_ns() - start >= gate->timeout_ns)
        {
            dvz_semaphore_signal(gate->semaphore, 1);
            break;
        }
        dvz_sleep(1);
    }
    return NULL;
}



/**
 * Check selected-queue completion without imposing unrelated-queue completion.
 *
 * @param suite active test context
 * @param independent whether to block an unrelated compute queue instead of the selected queue
 * @return zero on success
 */
static int _commands_queue_completion(TstContext* suite, bool independent)
{
    DvzGpuCtx* ctx = _commands_ctx(suite);
    ANN(ctx);
    DvzDevice* device = dvz_gpu_ctx_device(ctx);
    DvzQueue* main_queue = dvz_device_queue(device, DVZ_QUEUE_MAIN);
    DvzQueue* blocked_queue = main_queue;
    if (independent)
    {
        // Inspect actual allocated queues; role lookup may fall back to MAIN.
        blocked_queue = NULL;
        for (uint32_t i = 0; i < device->queues.queue_count; i++)
        {
            DvzQueue* queue = &device->queues.queues[i];
            if (dvz_queue_handle(queue) != dvz_queue_handle(main_queue))
            {
                blocked_queue = queue;
                break;
            }
        }
    }
    if (independent && (blocked_queue == NULL ||
                        dvz_queue_handle(blocked_queue) == dvz_queue_handle(main_queue)))
    {
        dvz_gpu_ctx_destroy(ctx);
        tst_skip(suite, "requires an independently allocated Vulkan queue");
        return 0;
    }
    DvzCommands* pending = dvz_commands_create_wrapper();
    DvzCommands* cmds = dvz_commands_create_wrapper();
    dvz_commands(device, blocked_queue, 1, pending);
    dvz_commands(device, main_queue, 1, cmds);
    dvz_cmd_begin(pending);
    dvz_cmd_end(pending);
    dvz_cmd_begin(cmds);
    dvz_cmd_end(cmds);
    DvzSemaphore* timeline = dvz_semaphore_create_wrapper();
    dvz_semaphore_timeline(device, 0, timeline, 0);
    DvzSubmit* submit = dvz_submit_create_wrapper();
    dvz_submit(submit);
    dvz_submit_command(submit, dvz_commands_handle(pending));
    dvz_submit_wait(submit, dvz_semaphore_handle(timeline), 1, VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT);
    dvz_submit_signal(submit, dvz_semaphore_handle(timeline), 2, VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT);
    int32_t submitted = dvz_submit_send(submit, dvz_queue_handle(blocked_queue), VK_NULL_HANDLE);
    AT(submitted == VK_SUCCESS);
    CommandsQueueGate gate = {
        .semaphore = timeline,
        .finished = dvz_atomic(),
        .timeout_ns = independent ? 2000000000ULL : 100000000ULL,
    };
    DvzThread* thread = dvz_thread(_commands_queue_gate_release, &gate);
    ANN(thread);
    int result = dvz_cmd_submit_result(cmds);
    uint64_t completed = dvz_semaphore_query(timeline);
    dvz_atomic_set(gate.finished, 1);
    dvz_thread_join(thread);
    if (dvz_semaphore_query(timeline) == 0)
        dvz_semaphore_signal(timeline, 1);
    dvz_queue_wait(blocked_queue);
    dvz_atomic_destroy(gate.finished);
    dvz_submit_free(submit);
    dvz_commands_destroy(pending);
    dvz_commands_free(pending);
    dvz_commands_destroy(cmds);
    dvz_commands_free(cmds);
    dvz_semaphore_destroy(timeline);
    dvz_semaphore_free(timeline);
    uint32_t errors = dvz_gpu_ctx_error_count(ctx);
    dvz_gpu_ctx_destroy(ctx);
    // Assertions follow release/cleanup, including when the old device wait timed out.
    AT(result == 0);
    AT(completed == (independent ? 0u : 2u));
    AT(errors == 0);
    return 0;
}



/**
 * Require synchronous completion of the selected queue.
 *
 * @param suite active test context
 * @param item active test item
 * @return zero on success
 */
int test_vklite_commands_waits_selected_queue(TstContext* suite, const TstCase* item)
{
    (void)item;
    return _commands_queue_completion(suite, false);
}



/**
 * Permit an unrelated queue to remain pending after synchronous submission.
 *
 * @param suite active test context
 * @param item active test item
 * @return zero on success
 */
int test_vklite_commands_does_not_wait_other_queue(TstContext* suite, const TstCase* item)
{
    (void)item;
    return _commands_queue_completion(suite, true);
}



/**
 * Prove the active Vulkan validation layer detects an intentionally missing dependency.
 *
 * @param suite active test context
 * @param item active test item
 * @return zero on success
 */
int test_vklite_synchronization_validation_active(TstContext* suite, const TstCase* item)
{
    (void)item;
    DvzGpuCtx* ctx = _commands_ctx(suite);
    ANN(ctx);
    DvzDevice* device = dvz_gpu_ctx_device(ctx);
    DvzBuffer* buffer = dvz_buffer_create_wrapper();
    dvz_buffer(device, dvz_gpu_ctx_alloc(ctx), buffer);
    dvz_buffer_size(buffer, 16);
    dvz_buffer_usage(buffer, VK_BUFFER_USAGE_TRANSFER_DST_BIT);
    int created = dvz_buffer_create(buffer);
    AT(created == 0);
    DvzCommands* cmds = dvz_commands_create_wrapper();
    dvz_commands(device, dvz_device_queue(device, DVZ_QUEUE_MAIN), 1, cmds);
    dvz_cmd_begin(cmds);
    VkCommandBuffer cmd = dvz_commands_handle(cmds);
    vkCmdFillBuffer(cmd, dvz_buffer_handle(buffer), 0, 16, 0);
    tst_log_capture_begin(suite);
    tst_expect_error_begin(suite);
    vkCmdFillBuffer(cmd, dvz_buffer_handle(buffer), 0, 16, 1);
    int expected_error = tst_expect_error_end(suite);
    bool hazard = false;
    for (uint32_t i = 0; i < tst_log_capture_count(suite); i++)
    {
        const TstLogRecord* record = tst_log_capture_get(suite, i);
        if (record != NULL &&
            (strstr(record->message, "WRITE_AFTER_WRITE") != NULL ||
             strstr(record->message, "SYNC-HAZARD-WRITE-AFTER-WRITE") != NULL))
            hazard = true;
    }
    tst_log_capture_end(suite);
    dvz_cmd_end(cmds);
    // Never submit the intentionally invalid stream to the GPU.
    dvz_commands_destroy(cmds);
    dvz_commands_free(cmds);
    dvz_buffer_destroy(buffer);
    dvz_buffer_free(buffer);
    uint32_t errors = dvz_gpu_ctx_error_count(ctx);
    dvz_gpu_ctx_destroy(ctx);
    AT(expected_error == 0);
    AT(hazard);
    AT(errors > 0);
    return 0;
}



int test_vklite_commands_destroy_idempotent(TstContext* suite, const TstCase* tstitem)
{
    ANN(suite);
    ANN(tstitem);

    DvzGpuCtx* ctx = _commands_ctx(suite);
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



int test_vklite_timeline_wait_blocks_until_signal(TstContext* suite, const TstCase* tstitem)
{
    ANN(suite);
    ANN(tstitem);

    DvzGpuCtx* ctx = _commands_ctx(suite);
    ANN(ctx);
    DvzDevice* device = dvz_gpu_ctx_device(ctx);
    ANN(device);

    DvzSemaphore* semaphore = dvz_semaphore_create_wrapper();
    ANN(semaphore);
    dvz_semaphore_timeline(device, 0, semaphore, 0);

    TimelineSignalData data = {
        .semaphore = semaphore,
        .value = 1,
        .delay_ms = 250,
    };
    DvzThread* thread = dvz_thread(_delayed_timeline_signal, &data);
    ANN(thread);

    uint64_t start = dvz_time_monotonic_ns();
    dvz_semaphore_wait(semaphore, data.value);
    uint64_t elapsed = dvz_time_monotonic_ns() - start;

    AT(dvz_semaphore_query(semaphore) >= data.value);
    AT(elapsed >= 200000000ULL);

    dvz_thread_join(thread);
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

    DvzGpuCtx* ctx = _commands_ctx(suite);
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

    DvzGpuCtx* ctx = _commands_ctx(suite);
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



int test_vklite_commands_borrowed_recording_unwrap(TstContext* suite, const TstCase* tstitem)
{
    ANN(suite);
    ANN(tstitem);

    DvzGpuCtx* ctx = _commands_ctx(suite);
    ANN(ctx);
    DvzDevice* device = dvz_gpu_ctx_device(ctx);
    ANN(device);

    DvzCommands* cmds = dvz_commands_create_wrapper();
    ANN(cmds);
    VkCommandBuffer first = (VkCommandBuffer)(uintptr_t)0x12345678;
    VkCommandBuffer second = (VkCommandBuffer)(uintptr_t)0x87654321;

    dvz_commands_wrap_borrowed_recording(device, first, cmds);
    AT(dvz_commands_handle(cmds) == first);
    AT(dvz_commands_unwrap(cmds) == DVZ_OK);
    AT(dvz_commands_count(cmds) == 0);
    AT(dvz_commands_handle(cmds) == VK_NULL_HANDLE);

    dvz_commands_wrap_borrowed_recording(device, second, cmds);
    AT(dvz_commands_handle(cmds) == second);
    AT(dvz_commands_unwrap(cmds) == DVZ_OK);

    dvz_commands_wrap(device, first, cmds);
    AT_EXPECTED_ERROR_STRICT(suite, dvz_commands_unwrap(cmds) == DVZ_ERROR);
    AT(dvz_commands_count(cmds) == 1);
    AT(dvz_commands_handle(cmds) == first);
    dvz_commands_destroy(cmds);
    AT_EXPECTED_ERROR_STRICT(suite, dvz_commands_unwrap(NULL) == DVZ_ERROR);
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

    DvzGpuCtx* ctx = _commands_ctx(suite);
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
