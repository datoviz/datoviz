/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Canvas public API                                                                            */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "canvas_internal.h"

#include <stdlib.h>
#include <string.h>
#if OS_UNIX
#include <unistd.h>
#endif

#include "_alloc.h"
#include "_assertions.h"
#include "_log.h"
#include "_time_utils.h"
#include "datoviz/fileio/fileio.h"
#include "datoviz/video.h"
#include "datoviz/vk/enums.h"
#include "datoviz/vk/queues.h"
#include "datoviz/vklite/buffers.h"
#include "datoviz/vklite/commands.h"
#include "datoviz/vklite/images.h"



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

static DvzStreamConfig canvas_stream_config(const DvzCanvas* canvas)
{
    ANN(canvas);
    DvzStreamConfig cfg = dvz_stream_default_config();
    if (canvas->surface)
    {
        cfg.width = canvas->surface->extent.width;
        cfg.height = canvas->surface->extent.height;
    }
    if (canvas->cfg.color_format != VK_FORMAT_UNDEFINED)
    {
        cfg.color_format = canvas->cfg.color_format;
    }
    return cfg;
}


static VkExternalMemoryHandleTypeFlagsKHR canvas_external_memory_handle_type(void);
static void canvas_init_offscreen_frame(const DvzCanvas* canvas, DvzStreamFrame* frame);



static bool canvas_is_offscreen_mode(const DvzCanvas* canvas)
{
    ANN(canvas);
    return canvas->cfg.render_mode == DVZ_CANVAS_RENDER_MODE_OFFSCREEN;
}



/**
 * Return whether video capture currently requires external-handle synchronization.
 *
 * @param canvas canvas instance
 * @return true when effective video capture mode is external, false otherwise
 */
static bool canvas_uses_external_video_capture(const DvzCanvas* canvas)
{
    ANN(canvas);
    if (!canvas->video_sink_enabled)
    {
        return false;
    }
    if (canvas->video_capture_mode == DVZ_VIDEO_CAPTURE_EXTERNAL)
    {
        return true;
    }
    return canvas->video_capture_mode == DVZ_VIDEO_CAPTURE_AUTO;
}



static const char* canvas_offscreen_state_name(DvzCanvasOffscreenRuntimeState state)
{
    switch (state)
    {
    case DVZ_CANVAS_OFFSCREEN_STATE_UNINITIALIZED:
        return "UNINITIALIZED";
    case DVZ_CANVAS_OFFSCREEN_STATE_READY:
        return "READY";
    case DVZ_CANVAS_OFFSCREEN_STATE_DRAW_PENDING:
        return "DRAW_PENDING";
    case DVZ_CANVAS_OFFSCREEN_STATE_OUTPUT_PENDING:
        return "OUTPUT_PENDING";
    case DVZ_CANVAS_OFFSCREEN_STATE_FATAL_DEVICE_LOST:
        return "FATAL_DEVICE_LOST";
    default:
        return "UNKNOWN";
    }
}



static void canvas_offscreen_transition(
    DvzCanvas* canvas, DvzCanvasOffscreenRuntimeState state, const char* reason)
{
    ANN(canvas);
    if (canvas->offscreen_runtime_state == state)
    {
        return;
    }
    log_debug(
        "canvas offscreen state %s -> %s (%s)",
        canvas_offscreen_state_name(canvas->offscreen_runtime_state),
        canvas_offscreen_state_name(state), reason ? reason : "no reason");
    canvas->offscreen_runtime_state = state;
}



static VkPipelineStageFlags2 canvas_stage_for_layout(VkImageLayout layout)
{
    switch (layout)
    {
    case VK_IMAGE_LAYOUT_UNDEFINED:
        return VK_PIPELINE_STAGE_2_NONE;
    case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
        return VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
    case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
        return VK_PIPELINE_STAGE_2_TRANSFER_BIT;
    case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
        return VK_PIPELINE_STAGE_2_TRANSFER_BIT;
    case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
        return VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
    case VK_IMAGE_LAYOUT_GENERAL:
        return VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
    default:
        return VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
    }
}



static VkAccessFlags2 canvas_access_for_layout(VkImageLayout layout)
{
    switch (layout)
    {
    case VK_IMAGE_LAYOUT_UNDEFINED:
        return 0;
    case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
        return VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT;
    case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
        return VK_ACCESS_2_TRANSFER_READ_BIT;
    case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
        return VK_ACCESS_2_TRANSFER_WRITE_BIT;
    case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
        return VK_ACCESS_2_SHADER_SAMPLED_READ_BIT;
    case VK_IMAGE_LAYOUT_GENERAL:
        return VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT;
    default:
        return 0;
    }
}



static void canvas_cmd_transition_image(
    DvzCanvas* canvas, VkCommandBuffer cmd, VkImage image, VkImageLayout old_layout,
    VkImageLayout new_layout)
{
    ANN(canvas);
    if (cmd == VK_NULL_HANDLE || image == VK_NULL_HANDLE || old_layout == new_layout)
    {
        return;
    }

    DvzCommands* cmds = dvz_commands_create_wrapper();
    ANN(cmds);
    dvz_commands_wrap(canvas->device, cmd, cmds);
    DvzBarriers barriers = {0};
    dvz_barriers(&barriers);
    DvzBarrierImage* bimg = dvz_barriers_image(&barriers, image);
    dvz_barrier_image_stage(
        bimg, canvas_stage_for_layout(old_layout), canvas_stage_for_layout(new_layout));
    dvz_barrier_image_access(
        bimg, canvas_access_for_layout(old_layout), canvas_access_for_layout(new_layout));
    dvz_barrier_image_layout(bimg, old_layout, new_layout);
    dvz_cmd_barriers(cmds, &barriers);
    dvz_commands_free(cmds);
}



static void canvas_offscreen_destroy_resources(DvzCanvas* canvas)
{
    if (!canvas || !canvas->offscreen_ready)
    {
        return;
    }
    dvz_device_wait(canvas->device);
    if (canvas->offscreen_views != NULL)
    {
        dvz_image_views_destroy(canvas->offscreen_views);
        dvz_image_views_free(canvas->offscreen_views);
        canvas->offscreen_views = NULL;
    }
    if (canvas->offscreen_images != NULL)
    {
        dvz_images_free(canvas->offscreen_images);
        canvas->offscreen_images = NULL;
    }
    if (canvas->offscreen_image != VK_NULL_HANDLE)
    {
        if (canvas->allocator != NULL && canvas->offscreen_alloc != NULL)
        {
            dvz_allocator_destroy_image(
                canvas->allocator, canvas->offscreen_alloc, canvas->offscreen_image);
            dvz_allocation_free(canvas->offscreen_alloc);
            canvas->offscreen_alloc = NULL;
        }
        canvas->offscreen_image = VK_NULL_HANDLE;
    }
    if (canvas->offscreen_command_buffer != VK_NULL_HANDLE)
    {
        dvz_command_buffer_free(canvas->device, canvas->offscreen_queue_family, canvas->offscreen_command_buffer);
        canvas->offscreen_command_buffer = VK_NULL_HANDLE;
    }
#if OS_UNIX
    if (canvas->offscreen_memory_fd >= 0)
    {
        close(canvas->offscreen_memory_fd);
        canvas->offscreen_memory_fd = -1;
    }
#endif
    canvas->offscreen_view = VK_NULL_HANDLE;
    canvas->offscreen_layout = VK_IMAGE_LAYOUT_UNDEFINED;
    canvas->offscreen_extent = (VkExtent2D){0, 0};
    canvas->offscreen_format = VK_FORMAT_UNDEFINED;
    canvas->offscreen_ready = false;
    canvas_offscreen_transition(canvas, DVZ_CANVAS_OFFSCREEN_STATE_UNINITIALIZED, "resources destroyed");
}



static int canvas_offscreen_create_resources(DvzCanvas* canvas, VkExtent2D extent, VkFormat format)
{
    ANN(canvas);
    if (extent.width == 0 || extent.height == 0)
    {
        log_error("offscreen canvas requires non-zero extent");
        return -1;
    }

    DvzQueue* queue_ref = dvz_device_queue(canvas->device, DVZ_QUEUE_MAIN);
    ANN(queue_ref);
    canvas->offscreen_queue_family = dvz_queue_family(queue_ref);
    canvas->offscreen_queue = dvz_queue_handle(queue_ref);
    canvas->offscreen_command_buffer =
        dvz_command_buffer_alloc(canvas->device, canvas->offscreen_queue_family);
    if (canvas->offscreen_command_buffer == VK_NULL_HANDLE)
    {
        log_error("failed to allocate offscreen canvas command buffer");
        return -1;
    }

    VkExternalMemoryImageCreateInfoKHR external_info = {
        .sType = VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO_KHR};
    VkImageCreateInfo img_info = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = format,
        .extent = {.width = extent.width, .height = extent.height, .depth = 1},
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
                 VK_IMAGE_USAGE_SAMPLED_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .pNext = NULL,
    };
    bool use_external = canvas->allocator != NULL && dvz_allocator_external(canvas->allocator) != 0;
    if (use_external)
    {
        external_info.handleTypes = dvz_allocator_external(canvas->allocator);
        img_info.pNext = &external_info;
    }

    canvas->offscreen_alloc = dvz_allocation_create();
    ANN(canvas->offscreen_alloc);
    if (dvz_allocator_image(
            canvas->allocator, &img_info, 0, canvas->offscreen_alloc, &canvas->offscreen_image) !=
        0)
    {
        dvz_allocation_free(canvas->offscreen_alloc);
        canvas->offscreen_alloc = NULL;
        log_error("failed to allocate offscreen canvas image");
        return -1;
    }

    if (canvas->offscreen_images == NULL)
    {
        canvas->offscreen_images = dvz_images_create_wrapper();
        ANN(canvas->offscreen_images);
    }
    if (canvas->offscreen_views == NULL)
    {
        canvas->offscreen_views = dvz_image_views_create_wrapper();
        ANN(canvas->offscreen_views);
    }
    dvz_images_wrap(
        canvas->device, canvas->allocator, VK_IMAGE_TYPE_2D, canvas->offscreen_image,
        canvas->offscreen_images);
    dvz_images_format(canvas->offscreen_images, format);
    dvz_image_views(canvas->offscreen_images, canvas->offscreen_views);
    dvz_image_views_create(canvas->offscreen_views);
    canvas->offscreen_view = dvz_image_views_handle(canvas->offscreen_views, 0);
    if (canvas->offscreen_view == VK_NULL_HANDLE)
    {
        log_error("failed to create offscreen canvas image view");
        canvas_offscreen_destroy_resources(canvas);
        return -1;
    }

    canvas->offscreen_memory_fd = -1;
    if (use_external && dvz_allocator_export(
                            canvas->allocator, canvas->offscreen_alloc,
                            &canvas->offscreen_memory_fd) != 0)
    {
        log_warn("failed to export offscreen canvas image memory handle");
        canvas->offscreen_memory_fd = -1;
    }

    canvas->offscreen_layout = VK_IMAGE_LAYOUT_UNDEFINED;
    canvas->offscreen_extent = extent;
    canvas->offscreen_format = format;
    canvas->offscreen_ready = true;
    return 0;
}



static int canvas_offscreen_prepare_frame(DvzCanvas* canvas, DvzStreamFrame* frame)
{
    ANN(canvas);
    ANN(frame);
    VkExtent2D extent = canvas->surface ? canvas->surface->extent : (VkExtent2D){0, 0};
    VkFormat format =
        canvas->cfg.color_format != VK_FORMAT_UNDEFINED ? canvas->cfg.color_format : DVZ_DEFAULT_COLOR_FORMAT;
    if (
        !canvas->offscreen_ready || canvas->offscreen_extent.width != extent.width ||
        canvas->offscreen_extent.height != extent.height || canvas->offscreen_format != format)
    {
        canvas_offscreen_destroy_resources(canvas);
        if (canvas_offscreen_create_resources(canvas, extent, format) != 0)
        {
            return -1;
        }
    }
    if (canvas->timeline_ready && canvas->timeline_value > 0)
    {
        dvz_semaphore_wait(canvas->timeline_semaphore, canvas->timeline_value);
    }

    DvzCommands* cmds = dvz_commands_create_wrapper();
    ANN(cmds);
    dvz_commands_wrap(canvas->device, canvas->offscreen_command_buffer, cmds);
    dvz_cmd_reset(cmds);
    dvz_cmd_begin(cmds);
    canvas_cmd_transition_image(
        canvas, canvas->offscreen_command_buffer, canvas->offscreen_image, canvas->offscreen_layout,
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    canvas->offscreen_layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    canvas_init_offscreen_frame(canvas, frame);
    frame->image = canvas->offscreen_image;
    frame->image_view = canvas->offscreen_view;
    frame->command_buffer = canvas->offscreen_command_buffer;
    frame->extent = canvas->offscreen_extent;
    frame->memory = VK_NULL_HANDLE;
    frame->memory_size = 0;
    frame->memory_fd = canvas->offscreen_memory_fd;
    frame->handles_dirty = false;
    dvz_commands_free(cmds);
    return 0;
}

static bool canvas_device_check_extensions(DvzCanvas* canvas)
{
    ANN(canvas);
    ANN(canvas->device);

    const char* const required_extensions[] = {
        canvas_is_offscreen_mode(canvas) ? NULL : VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    };
    const size_t required_count = sizeof(required_extensions) / sizeof(required_extensions[0]);
    bool ok = true;
    for (size_t i = 0; i < required_count; ++i)
    {
        const char* name = required_extensions[i];
        if (!name || name[0] == '\0')
        {
            continue;
        }
        if (!dvz_device_has_extension(canvas->device, name))
        {
            log_error("canvas device missing required extension '%s'", name);
            ok = false;
        }
    }

    canvas->supports_external_memory = false;
    VkExternalMemoryHandleTypeFlagsKHR mem_handle = canvas_external_memory_handle_type();
    if (mem_handle != 0 &&
        dvz_device_has_extension(canvas->device, VK_KHR_EXTERNAL_MEMORY_EXTENSION_NAME))
    {
        if (
#if OS_UNIX
            dvz_device_has_extension(canvas->device, VK_KHR_EXTERNAL_MEMORY_FD_EXTENSION_NAME)
#elif OS_WINDOWS
            dvz_device_has_extension(canvas->device, VK_KHR_EXTERNAL_MEMORY_WIN32_EXTENSION_NAME)
#endif
        )
        {
            canvas->supports_external_memory = true;
        }
    }

    canvas->supports_external_semaphore = false;
    VkExternalSemaphoreHandleTypeFlags sem_handle = dvz_canvas_timeline_handle_type();
    if (sem_handle != 0 &&
        dvz_device_has_extension(canvas->device, VK_KHR_EXTERNAL_SEMAPHORE_EXTENSION_NAME))
    {
        if (
#if OS_UNIX
            dvz_device_has_extension(canvas->device, VK_KHR_EXTERNAL_SEMAPHORE_FD_EXTENSION_NAME)
#elif OS_WINDOWS
            dvz_device_has_extension(
                canvas->device, VK_KHR_EXTERNAL_SEMAPHORE_WIN32_EXTENSION_NAME)
#endif
        )
        {
            canvas->supports_external_semaphore = true;
        }
    }

    if (canvas->cfg.enable_video_sink &&
        (!canvas->supports_external_memory || !canvas->supports_external_semaphore))
    {
        log_warn("video sink requested but required external memory/semaphore extensions missing");
    }

    return ok;
}



static VkExternalMemoryHandleTypeFlagsKHR canvas_external_memory_handle_type(void)
{
#if OS_MACOS
    return 0;
#elif OS_LINUX
    return VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT;
#elif OS_WINDOWS
    return VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT;
#else
    return 0;
#endif
}



VkExternalSemaphoreHandleTypeFlags dvz_canvas_timeline_handle_type(void)
{
#if OS_MACOS
    return 0;
#elif OS_LINUX
    return VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_FD_BIT;
#elif OS_WINDOWS
    return VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_WIN32_BIT;
#else
    return 0;
#endif
}



static int canvas_create_allocator(DvzCanvas* canvas)
{
    ANN(canvas);
    if (canvas->allocator_ready)
    {
        return 0;
    }
    if (canvas->allocator == NULL)
    {
        canvas->allocator = dvz_allocator_create();
        ANN(canvas->allocator);
    }
    VkExternalMemoryHandleTypeFlagsKHR handle_type =
        canvas->supports_external_memory ? canvas_external_memory_handle_type() : 0;
    if (dvz_device_allocator(canvas->device, handle_type, canvas->allocator) != 0)
    {
        log_error("failed to create canvas allocator");
        return -1;
    }
    canvas->allocator_ready = true;
    return 0;
}



static void canvas_destroy_allocator(DvzCanvas* canvas)
{
    if (!canvas || !canvas->allocator_ready)
    {
        return;
    }
    if (canvas->allocator != NULL)
    {
        dvz_allocator_destroy(canvas->allocator);
        dvz_allocator_free(canvas->allocator);
        canvas->allocator = NULL;
    }
    canvas->allocator_ready = false;
}



static int canvas_create_timeline(DvzCanvas* canvas)
{
    ANN(canvas);
    if (canvas->timeline_ready)
    {
        return 0;
    }

    VkExternalSemaphoreHandleTypeFlags handle_type =
        canvas->supports_external_semaphore ? dvz_canvas_timeline_handle_type() : 0;
    if (canvas->timeline_semaphore == NULL)
    {
        canvas->timeline_semaphore = dvz_semaphore_create_wrapper();
        ANN(canvas->timeline_semaphore);
    }
    dvz_semaphore_timeline(canvas->device, 0, canvas->timeline_semaphore, handle_type);
#if OS_UNIX
    if (handle_type != 0)
    {
        int probe_fd = dvz_semaphore_export_fd(canvas->timeline_semaphore, handle_type);
        if (probe_fd >= 0)
        {
            close(probe_fd);
        }
        else
        {
            log_warn("canvas timeline semaphore export probe failed; disabling external semaphore path");
            canvas->supports_external_semaphore = false;
        }
    }
#endif


    canvas->timeline_ready = true;
    canvas->timeline_value = 0;
    return 0;
}



static int canvas_prepare_video_wait_semaphore_fd(DvzCanvas* canvas, DvzStreamFrame* frame)
{
    ANN(canvas);
    ANN(frame);

    if (!canvas_uses_external_video_capture(canvas))
    {
        return 0;
    }
    if (!canvas->timeline_ready || !canvas->supports_external_semaphore)
    {
        log_error("video sink timeline export unavailable");
        return -1;
    }

#if OS_UNIX
    VkExternalSemaphoreHandleTypeFlags handle_type = dvz_canvas_timeline_handle_type();
    int fd = -1;
    if (!canvas->test_force_wait_semaphore_export_failure)
    {
        fd = dvz_semaphore_export_fd(canvas->timeline_semaphore, handle_type);
    }
    if (frame->wait_semaphore_fd >= 0)
    {
        close(frame->wait_semaphore_fd);
        frame->wait_semaphore_fd = -1;
    }
    if (fd < 0)
    {
        if (!canvas->test_force_wait_semaphore_export_failure)
        {
            log_warn("video sink timeline export failed, continuing without wait semaphore handle");
        }
        return 0;
    }
    frame->wait_semaphore_fd = fd;
    return 0;
#else
    log_error("video sink timeline export unsupported on this platform");
    return -1;
#endif
}



/**
 * Force timeline wait-semaphore FD export failure in tests.
 *
 * @param canvas canvas whose test flag should be updated
 * @param enabled true to force export failure, false to restore normal behavior
 */
void dvz_canvas_test_force_wait_semaphore_export_failure(DvzCanvas* canvas, bool enabled)
{
    if (!canvas)
    {
        return;
    }
    canvas->test_force_wait_semaphore_export_failure = enabled;
}



/**
 * Force an offscreen submit status in tests.
 *
 * @param canvas canvas whose submit hook should be updated
 * @param status VkResult value encoded as int32_t, or -1 to disable forcing
 */
void dvz_canvas_test_force_offscreen_submit_status(DvzCanvas* canvas, int32_t status)
{
    if (!canvas)
    {
        return;
    }
    canvas->test_force_offscreen_submit_status = status;
    canvas->test_force_offscreen_submit_status_set = true;
}



static void canvas_destroy_timeline(DvzCanvas* canvas)
{
    if (!canvas || !canvas->timeline_ready)
    {
        return;
    }
    dvz_semaphore_destroy(canvas->timeline_semaphore);
    dvz_semaphore_free(canvas->timeline_semaphore);
    canvas->timeline_semaphore = NULL;
    canvas->timeline_ready = false;
}



/*************************************************************************************************/
/*  Frame pool                                                                                   */
/*************************************************************************************************/

void dvz_canvas_frame_pool_init(DvzCanvasFramePool* pool, uint32_t frame_count)
{
    ANN(pool);
    dvz_canvas_frame_pool_release(pool);
    pool->frame_count = frame_count > 0 ? frame_count : 1;
    pool->frames = (DvzStreamFrame*)dvz_calloc(pool->frame_count, sizeof(DvzStreamFrame));
    ANN(pool->frames);
    for (uint32_t i = 0; i < pool->frame_count; i++)
    {
        pool->frames[i].memory_fd = -1;
        pool->frames[i].wait_semaphore_fd = -1;
    }
    pool->current_index = 0;
}



void dvz_canvas_frame_pool_release(DvzCanvasFramePool* pool)
{
    if (!pool)
    {
        return;
    }
    if (pool->frames)
    {
#if OS_UNIX
        for (uint32_t i = 0; i < pool->frame_count; i++)
        {
            if (pool->frames[i].wait_semaphore_fd >= 0)
            {
                close(pool->frames[i].wait_semaphore_fd);
                pool->frames[i].wait_semaphore_fd = -1;
            }
        }
#endif
        dvz_free(pool->frames);
        pool->frames = NULL;
    }
    pool->frame_count = 0;
    pool->current_index = 0;
}



DvzStreamFrame* dvz_canvas_frame_pool_current(DvzCanvasFramePool* pool)
{
    ANN(pool);
    if (!pool->frames || pool->frame_count == 0)
    {
        return NULL;
    }
    return &pool->frames[pool->current_index];
}



DvzStreamFrame* dvz_canvas_frame_pool_rotate(DvzCanvasFramePool* pool)
{
    ANN(pool);
    if (!pool->frames || pool->frame_count == 0)
    {
        return NULL;
    }
    pool->current_index = (pool->current_index + 1) % pool->frame_count;
    return &pool->frames[pool->current_index];
}



/*************************************************************************************************/
/*  Timings                                                                                      */
/*************************************************************************************************/

void dvz_canvas_timings_init(DvzCanvasTimingState* timings, size_t capacity)
{
    ANN(timings);
    dvz_canvas_timings_release(timings);
    timings->capacity = capacity;
    timings->count = 0;
    timings->head = 0;
    if (capacity > 0)
    {
        timings->samples = (DvzFrameTiming*)dvz_calloc(capacity, sizeof(DvzFrameTiming));
        ANN(timings->samples);
    }
}



void dvz_canvas_timings_release(DvzCanvasTimingState* timings)
{
    if (!timings)
    {
        return;
    }
    if (timings->samples)
    {
        dvz_free(timings->samples);
        timings->samples = NULL;
    }
    timings->capacity = 0;
    timings->count = 0;
    timings->head = 0;
}



void dvz_canvas_timings_record(
    DvzCanvasTimingState* timings, uint64_t frame_id, double cpu_submit_us)
{
    ANN(timings);
    if (!timings->samples || timings->capacity == 0)
    {
        return;
    }
    DvzFrameTiming* timing = &timings->samples[timings->head];
    *timing = (DvzFrameTiming){
        .frame_id = frame_id,
        .cpu_submit_us = cpu_submit_us,
        .gpu_complete_us = 0.0,
        .present_start_us = 0.0,
        .present_done_us = 0.0,
    };
    timings->head = (timings->head + 1) % timings->capacity;
    if (timings->count < timings->capacity)
    {
        timings->count++;
    }
}



const DvzFrameTiming* dvz_canvas_timings_view(const DvzCanvasTimingState* timings, size_t* count)
{
    ANN(timings);
    if (count)
    {
        *count = timings->count;
    }
    return timings->samples;
}



static void canvas_init_offscreen_frame(const DvzCanvas* canvas, DvzStreamFrame* frame)
{
    ANN(canvas);
    ANN(frame);
    dvz_memset(frame, sizeof(*frame), 0, sizeof(*frame));
    frame->memory_fd = -1;
    frame->wait_semaphore_fd = -1;
    if (canvas->surface)
    {
        frame->extent = canvas->surface->extent;
    }
}



static int canvas_offscreen_capture_rgba_into(
    DvzCanvas* canvas, uint32_t width, uint32_t height, uint8_t* out_rgba, size_t out_size)
{
    ANN(canvas);
    ANN(out_rgba);
    if (!canvas->offscreen_ready || canvas->offscreen_image == VK_NULL_HANDLE)
    {
        log_error("offscreen canvas capture requires a prepared frame");
        return -1;
    }
    if (width != canvas->offscreen_extent.width || height != canvas->offscreen_extent.height)
    {
        log_error(
            "offscreen capture dimension mismatch, expected %ux%u but got %ux%u",
            canvas->offscreen_extent.width, canvas->offscreen_extent.height, width, height);
        return -1;
    }
    size_t expected_size = (size_t)width * (size_t)height * 4;
    if (out_size < expected_size)
    {
        log_error(
            "offscreen capture destination buffer too small (%zu < %zu)", out_size,
            expected_size);
        return -1;
    }
    if (canvas->timeline_value > 0)
    {
        dvz_semaphore_wait(canvas->timeline_semaphore, canvas->timeline_value);
    }

    DvzBuffer* staging = dvz_buffer_create_wrapper();
    ANN(staging);
    dvz_buffer(canvas->device, canvas->allocator, staging);
    dvz_buffer_size(staging, expected_size);
    dvz_buffer_flags(
        staging, VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT);
    dvz_buffer_usage(staging, VK_BUFFER_USAGE_TRANSFER_DST_BIT);
    if (dvz_buffer_create(staging) != 0)
    {
        dvz_buffer_free(staging);
        log_error("failed to allocate staging buffer for offscreen capture");
        return -1;
    }

    VkCommandBuffer cmd = dvz_command_buffer_alloc(canvas->device, canvas->offscreen_queue_family);
    if (cmd == VK_NULL_HANDLE)
    {
        dvz_buffer_destroy(staging);
        dvz_buffer_free(staging);
        log_error("failed to allocate command buffer for offscreen capture");
        return -1;
    }

    DvzCommands* cmds = dvz_commands_create_wrapper();
    ANN(cmds);
    dvz_commands_wrap(canvas->device, cmd, cmds);
    dvz_cmd_reset(cmds);
    dvz_cmd_begin(cmds);
    VkImageLayout original_layout = canvas->offscreen_layout;
    if (original_layout == VK_IMAGE_LAYOUT_UNDEFINED)
    {
        original_layout = VK_IMAGE_LAYOUT_GENERAL;
    }
    canvas_cmd_transition_image(
        canvas, cmd, canvas->offscreen_image, original_layout, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
    DvzImageRegion region = {0};
    dvz_image_region(&region);
    dvz_image_region_extent(&region, width, height, 1);
    dvz_cmd_copy_image_to_buffer(
        cmds, canvas->offscreen_image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, &region,
        dvz_buffer_handle(staging), 0);
    canvas_cmd_transition_image(
        canvas, cmd, canvas->offscreen_image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, original_layout);
    dvz_cmd_end(cmds);
    dvz_commands_free(cmds);

    DvzFence* fence = dvz_fence_create_wrapper();
    DvzSubmit* submit = dvz_submit_create_wrapper();
    ANN(fence);
    ANN(submit);
    dvz_fence(canvas->device, false, fence);
    dvz_submit(submit);
    dvz_submit_command(submit, cmd);
    int32_t submit_rc = dvz_submit_send(submit, canvas->offscreen_queue, dvz_fence_handle(fence));
    if (submit_rc != VK_SUCCESS)
    {
        dvz_fence_destroy(fence);
        dvz_fence_free(fence);
        dvz_submit_free(submit);
        dvz_command_buffer_free(canvas->device, canvas->offscreen_queue_family, cmd);
        dvz_buffer_destroy(staging);
        dvz_buffer_free(staging);
        log_error("failed to submit offscreen capture copy commands (%d)", submit_rc);
        return -1;
    }
    dvz_fence_wait(fence);
    dvz_fence_destroy(fence);
    dvz_fence_free(fence);
    dvz_submit_free(submit);
    dvz_command_buffer_free(canvas->device, canvas->offscreen_queue_family, cmd);

    dvz_buffer_download(staging, 0, expected_size, out_rgba);
    dvz_buffer_destroy(staging);
    dvz_buffer_free(staging);
    return 0;
}



/*************************************************************************************************/
/*  Public API                                                                                   */
/*************************************************************************************************/

/**
 * Return the default canvas configuration.
 *
 * @returns a configuration initialized with sensible defaults
 */
DvzCanvasConfig dvz_canvas_default_config(void)
{
    DvzCanvasConfig cfg = {
        .window = NULL,
        .device = NULL,
        .render_mode = DVZ_CANVAS_RENDER_MODE_PRESENT,
        .color_format = VK_FORMAT_UNDEFINED,
        .present_mode = VK_PRESENT_MODE_FIFO_KHR,
        .enable_video_sink = false,
        .timing_history = DVZ_CANVAS_DEFAULT_TIMING_HISTORY,
    };
    return cfg;
}



/**
 * Create a canvas instance associated with the provided window/device pair.
 *
 * @param cfg configuration describing the canvas requirements
 */
DvzCanvas* dvz_canvas_create(const DvzCanvasConfig* cfg)
{
    DvzCanvasConfig resolved = cfg ? *cfg : dvz_canvas_default_config();
    if (
        resolved.render_mode != DVZ_CANVAS_RENDER_MODE_PRESENT &&
        resolved.render_mode != DVZ_CANVAS_RENDER_MODE_OFFSCREEN)
    {
        resolved.render_mode = DVZ_CANVAS_RENDER_MODE_PRESENT;
    }
    if (!resolved.window)
    {
        log_error("canvas creation requires a valid window handle");
        return NULL;
    }
    if (!resolved.device)
    {
        log_error("canvas creation requires a valid device handle");
        return NULL;
    }
    if (
        resolved.render_mode == DVZ_CANVAS_RENDER_MODE_PRESENT &&
        dvz_window_backend_type(resolved.window) == DVZ_BACKEND_OFFSCREEN)
    {
        log_error("present render mode requires a presentation-capable window backend");
        return NULL;
    }

    DvzCanvas* canvas = (DvzCanvas*)dvz_calloc(1, sizeof(DvzCanvas));
    ANN(canvas);
    canvas->cfg = resolved;
    canvas->window = resolved.window;
    canvas->device = resolved.device;
    canvas->sink_registry = NULL;
    canvas->draw_callback = NULL;
    canvas->draw_user_data = NULL;
    canvas->frame_id = 0;
    canvas->video_sink_enabled = false;
    canvas->video_sink_cfg_valid = false;
    canvas->live_image_sink_enabled = false;
    canvas->stream_started = false;
    canvas->primary_sink_attached = false;
    canvas->offscreen_memory_fd = -1;
    canvas->offscreen_ready = false;
    canvas->offscreen_runtime_state = DVZ_CANVAS_OFFSCREEN_STATE_UNINITIALIZED;
    canvas->test_force_wait_semaphore_export_failure = false;
    canvas->test_force_offscreen_submit_status = -1;
    canvas->test_force_offscreen_submit_status_set = false;

    if (!canvas_device_check_extensions(canvas))
    {
        log_error("canvas device missing required extensions");
        dvz_canvas_destroy(canvas);
        return NULL;
    }

    if (canvas_create_allocator(canvas) != 0 || canvas_create_timeline(canvas) != 0)
    {
        dvz_canvas_destroy(canvas);
        return NULL;
    }

    dvz_canvas_window_surface_refresh(canvas);
    canvas->sink_registry = dvz_stream_sink_registry_create();
    if (canvas->sink_registry == NULL)
    {
        log_error("failed to create canvas sink registry");
        dvz_canvas_destroy(canvas);
        return NULL;
    }
    DvzStreamConfig stream_cfg = canvas_stream_config(canvas);
    canvas->stream = dvz_stream_create(canvas->device, canvas->sink_registry, &stream_cfg);
    if (!canvas->stream)
    {
        log_error("failed to allocate stream for canvas");
        dvz_canvas_destroy(canvas);
        return NULL;
    }

    dvz_canvas_frame_pool_init(&canvas->frame_pool, 1);
    size_t timing_history =
        resolved.timing_history > 0 ? resolved.timing_history : DVZ_CANVAS_DEFAULT_TIMING_HISTORY;
    dvz_canvas_timings_init(&canvas->timings, timing_history);

    if (dvz_canvas_stream_prepare(canvas) != 0)
    {
        log_warn("canvas stream preparation failed; primary sink unavailable");
    }

    if (resolved.enable_video_sink)
    {
        dvz_canvas_configure_video_sink(canvas, true, NULL);
    }

    if (
        !canvas_is_offscreen_mode(canvas) &&
        dvz_canvas_swapchain_init(canvas) != 0)
    {
        log_error("failed to initialize canvas swapchain state");
        dvz_canvas_destroy(canvas);
        return NULL;
    }
    if (canvas_is_offscreen_mode(canvas))
    {
        canvas_offscreen_transition(canvas, DVZ_CANVAS_OFFSCREEN_STATE_READY, "initialized");
    }
    return canvas;
}



/**
 * Destroy a canvas and its owned resources.
 *
 * @param canvas canvas returned by dvz_canvas_create()
 */
void dvz_canvas_destroy(DvzCanvas* canvas)
{
    if (!canvas)
    {
        return;
    }
    dvz_canvas_swapchain_destroy(canvas);
    if (canvas->stream)
    {
        dvz_stream_stop(canvas->stream);
    }
    dvz_canvas_stream_enable_video(canvas, false, NULL);
    if (canvas->stream)
    {
        dvz_stream_destroy(canvas->stream);
        canvas->stream = NULL;
    }
    if (canvas->sink_registry != NULL)
    {
        dvz_stream_sink_registry_destroy(canvas->sink_registry);
        canvas->sink_registry = NULL;
    }
    canvas_offscreen_destroy_resources(canvas);
    canvas_destroy_timeline(canvas);
    canvas_destroy_allocator(canvas);
    dvz_canvas_frame_pool_release(&canvas->frame_pool);
    dvz_canvas_timings_release(&canvas->timings);
    dvz_free(canvas);
}



/**
 * Register a draw callback invoked during dvz_canvas_frame().
 *
 * @param canvas canvas whose callback should be updated
 * @param callback draw callback pointer
 * @param user_data user data forwarded to the callback
 */
void dvz_canvas_set_draw_callback(DvzCanvas* canvas, DvzCanvasDraw callback, void* user_data)
{
    ANN(canvas);
    canvas->draw_callback = callback;
    canvas->draw_user_data = user_data;
}



/**
 * Acquire the next frame and execute the registered draw callback.
 *
 * @param canvas canvas to update
 * @returns 0 when the frame is ready or -1 on error
 */
int dvz_canvas_frame(DvzCanvas* canvas)
{
    ANN(canvas);
    dvz_canvas_window_surface_refresh(canvas);
    if (dvz_canvas_stream_prepare(canvas) != 0)
    {
        return -1;
    }

    DvzStreamFrame* frame = NULL;
    if (canvas_is_offscreen_mode(canvas))
    {
        if (canvas->offscreen_runtime_state == DVZ_CANVAS_OFFSCREEN_STATE_FATAL_DEVICE_LOST)
        {
            log_error("offscreen canvas frame aborted after device loss");
            return -1;
        }
        frame = dvz_canvas_frame_pool_rotate(&canvas->frame_pool);
        if (!frame)
        {
            log_error("canvas frame pool unavailable");
            return -1;
        }
        canvas_offscreen_transition(canvas, DVZ_CANVAS_OFFSCREEN_STATE_DRAW_PENDING, "frame begin");
        if (canvas_offscreen_prepare_frame(canvas, frame) != 0)
        {
            canvas_offscreen_transition(canvas, DVZ_CANVAS_OFFSCREEN_STATE_READY, "frame prepare failed");
            log_error("failed to prepare offscreen canvas frame");
            return -1;
        }
    }
    else
    {
        DvzStreamFrame frame_data = {0};
        int acquire_rc = dvz_canvas_swapchain_acquire(canvas, &frame_data);
        if (acquire_rc == DVZ_CANVAS_FRAME_WAIT_SURFACE)
        {
            return DVZ_CANVAS_FRAME_WAIT_SURFACE;
        }
        if (acquire_rc != 0)
        {
            log_warn("unable to acquire canvas frame from swapchain");
            return -1;
        }
        frame = dvz_canvas_frame_pool_rotate(&canvas->frame_pool);
        if (!frame)
        {
            log_error("canvas frame pool unavailable");
            return -1;
        }
        *frame = frame_data;
    }

    // Sync-handle ordering contract: prepare the timeline wait handle before stream start/update so
    // video sinks always see the latest semaphore handle during their start/update callback.
    bool needs_video_sync_refresh =
        canvas_uses_external_video_capture(canvas) &&
        (!canvas->stream_started || frame->handles_dirty);
    if (needs_video_sync_refresh && canvas_prepare_video_wait_semaphore_fd(canvas, frame) != 0)
    {
        return -1;
    }

    bool stream_was_started = canvas->stream_started;
    if (dvz_canvas_stream_start(canvas, frame) != 0)
    {
        return -1;
    }
    // If the stream starts on this frame, sinks consumed the frame in start(); otherwise a handle
    // refresh must be propagated through update() before the next submit().
    bool stream_started_now = !stream_was_started && canvas->stream_started;
    if (canvas_is_offscreen_mode(canvas))
    {
        // Offscreen contract: frame metadata stays stable within a stream lifecycle, so canvas
        // never routes per-frame metadata through stream->update() in this mode.
        if (stream_started_now && frame->handles_dirty)
        {
            frame->handles_dirty = false;
        }
        else if (frame->handles_dirty)
        {
            log_warn("offscreen frame reported dirty handles; clearing without stream update");
            frame->handles_dirty = false;
        }
    }
    else if (stream_started_now)
    {
        dvz_canvas_swapchain_handles_refreshed(canvas);
        frame->handles_dirty = false;
    }
    else if (canvas->stream_started && frame->handles_dirty)
    {
        if (dvz_stream_update(canvas->stream, frame) != 0)
        {
            log_error("failed to refresh canvas stream frame handles");
            return -1;
        }
        dvz_canvas_swapchain_handles_refreshed(canvas);
        frame->handles_dirty = false;
    }

    if (canvas->draw_callback)
    {
        canvas->draw_callback(canvas, frame, canvas->draw_user_data);
    }
    canvas->frame_id++;
    return DVZ_CANVAS_FRAME_READY;
}



/**
 * Submit the current frame to the canvas stream.
 *
 * @param canvas canvas to submit
 * @returns 0 on success, negative error otherwise
 */
int dvz_canvas_submit(DvzCanvas* canvas)
{
    ANN(canvas);
    DvzStreamFrame* frame = dvz_canvas_frame_pool_current(&canvas->frame_pool);
    if (!frame)
    {
        log_error("no canvas frame available for submission");
        return -1;
    }

    DvzClock clock = dvz_clock();
    dvz_clock_tick(&clock);
    uint64_t wait_value = canvas->timeline_value + 1;
    bool offscreen_submit_signaled = false;
    if (canvas_is_offscreen_mode(canvas))
    {
        if (canvas->offscreen_runtime_state == DVZ_CANVAS_OFFSCREEN_STATE_FATAL_DEVICE_LOST)
        {
            log_error("offscreen canvas submit aborted after device loss");
            return -1;
        }
        if (!canvas->offscreen_ready || canvas->offscreen_command_buffer == VK_NULL_HANDLE)
        {
            log_error("offscreen canvas submit requires prepared offscreen resources");
            return -1;
        }
        canvas_offscreen_transition(
            canvas, DVZ_CANVAS_OFFSCREEN_STATE_OUTPUT_PENDING, "submit begin");
        canvas_cmd_transition_image(
            canvas, canvas->offscreen_command_buffer, canvas->offscreen_image, canvas->offscreen_layout,
            VK_IMAGE_LAYOUT_GENERAL);
        canvas->offscreen_layout = VK_IMAGE_LAYOUT_GENERAL;
        DvzCommands* cmds = dvz_commands_create_wrapper();
        ANN(cmds);
        dvz_commands_wrap(canvas->device, canvas->offscreen_command_buffer, cmds);
        dvz_cmd_end(cmds);
        dvz_commands_free(cmds);

        DvzSubmit* submit = dvz_submit_create_wrapper();
        ANN(submit);
        dvz_submit(submit);
        dvz_submit_command(submit, canvas->offscreen_command_buffer);
        dvz_submit_signal(
            submit, dvz_semaphore_handle(canvas->timeline_semaphore), wait_value,
            VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT);
        int32_t submit_rc = 0;
        if (canvas->test_force_offscreen_submit_status_set)
        {
            submit_rc = canvas->test_force_offscreen_submit_status;
            canvas->test_force_offscreen_submit_status_set = false;
            canvas->test_force_offscreen_submit_status = -1;
        }
        else
        {
            submit_rc = dvz_submit_send(submit, canvas->offscreen_queue, VK_NULL_HANDLE);
        }
        dvz_submit_free(submit);
        if (submit_rc != VK_SUCCESS)
        {
            if (submit_rc == VK_ERROR_DEVICE_LOST)
            {
                canvas_offscreen_transition(
                    canvas, DVZ_CANVAS_OFFSCREEN_STATE_FATAL_DEVICE_LOST, "submit device lost");
            }
            else
            {
                canvas_offscreen_transition(
                    canvas, DVZ_CANVAS_OFFSCREEN_STATE_DRAW_PENDING, "submit failed");
            }
            log_error("offscreen canvas submit failed (%d)", submit_rc);
            return -1;
        }
        offscreen_submit_signaled = true;
    }
    int result = dvz_canvas_stream_submit(canvas, wait_value);
    if (canvas_is_offscreen_mode(canvas))
    {
        if (offscreen_submit_signaled)
        {
            canvas->timeline_value = wait_value;
        }
        if (result == 0)
        {
            canvas_offscreen_transition(canvas, DVZ_CANVAS_OFFSCREEN_STATE_READY, "submit done");
        }
        else
        {
            canvas_offscreen_transition(
                canvas, DVZ_CANVAS_OFFSCREEN_STATE_DRAW_PENDING, "stream submit failed");
        }
    }
    else if (result == 0)
    {
        canvas->timeline_value = wait_value;
    }
    double elapsed = dvz_clock_interval(&clock) * 1e6;
    dvz_canvas_timings_record(&canvas->timings, canvas->frame_id, elapsed);
    return result;
}



/**
 * Return the render mode currently configured on the canvas.
 *
 * @param canvas canvas handle
 * @returns canvas render mode
 */
DvzCanvasRenderMode dvz_canvas_render_mode(const DvzCanvas* canvas)
{
    ANN(canvas);
    return canvas->cfg.render_mode;
}



/**
 * Return the current present runtime state for diagnostics/tests.
 *
 * @param canvas canvas handle
 * @returns present runtime state or UNINITIALIZED when unavailable
 */
DvzCanvasPresentRuntimeState dvz_canvas_present_runtime_state(const DvzCanvas* canvas)
{
    if (!canvas)
    {
        return DVZ_CANVAS_PRESENT_STATE_UNINITIALIZED;
    }
    return dvz_canvas_swapchain_runtime_state(canvas);
}



/**
 * Return the current offscreen runtime state for diagnostics/tests.
 *
 * @param canvas canvas handle
 * @returns offscreen runtime state or UNINITIALIZED when canvas is null
 */
DvzCanvasOffscreenRuntimeState dvz_canvas_offscreen_runtime_state(const DvzCanvas* canvas)
{
    if (!canvas)
    {
        return DVZ_CANVAS_OFFSCREEN_STATE_UNINITIALIZED;
    }
    return canvas->offscreen_runtime_state;
}



/**
 * Return the input router tied to the canvas window.
 *
 * @param canvas canvas owning the router
 */
DvzInputRouter* dvz_canvas_input(DvzCanvas* canvas)
{
    ANN(canvas);
    if (!canvas->window)
    {
        return NULL;
    }
    return dvz_window_router(canvas->window);
}



/**
 * Capture the latest presented frame into caller-managed RGBA storage.
 *
 * @param canvas canvas to capture
 * @param width expected frame width
 * @param height expected frame height
 * @param out_rgba destination RGBA buffer
 * @param out_size destination buffer size in bytes
 * @returns 0 on success or a negative error code
 */
int dvz_canvas_capture_rgba_into(
    DvzCanvas* canvas, uint32_t width, uint32_t height, uint8_t* out_rgba, size_t out_size)
{
    ANN(canvas);
    ANN(out_rgba);
    if (canvas_is_offscreen_mode(canvas))
    {
        return canvas_offscreen_capture_rgba_into(canvas, width, height, out_rgba, out_size);
    }
    if (width == 0 || height == 0)
    {
        log_error("canvas capture requires non-zero dimensions");
        return -1;
    }
    return dvz_canvas_swapchain_capture_rgba_into(canvas, width, height, out_rgba, out_size);
}



/**
 * Capture the latest presented frame into a newly allocated RGBA buffer.
 *
 * @param canvas canvas to capture
 * @param out_width destination width
 * @param out_height destination height
 * @param out_rgba destination pointer receiving allocated RGBA pixels
 * @returns 0 on success or a negative error code
 */
int dvz_canvas_capture_rgba(
    DvzCanvas* canvas, uint32_t* out_width, uint32_t* out_height, uint8_t** out_rgba)
{
    ANN(canvas);
    ANN(out_width);
    ANN(out_height);
    ANN(out_rgba);
    *out_width = 0;
    *out_height = 0;
    *out_rgba = NULL;

    DvzCanvasSurfaceInfo surface = dvz_canvas_window_surface_info(canvas);
    uint32_t width = surface.extent.width;
    uint32_t height = surface.extent.height;
    if (width == 0 || height == 0)
    {
        log_error("canvas capture requires a non-zero surface extent");
        return -1;
    }

    size_t byte_count = (size_t)width * (size_t)height * 4;
    uint8_t* rgba = (uint8_t*)dvz_calloc(byte_count, sizeof(uint8_t));
    ANN(rgba);
    if (dvz_canvas_capture_rgba_into(canvas, width, height, rgba, byte_count) != 0)
    {
        dvz_free(rgba);
        return -1;
    }

    *out_width = width;
    *out_height = height;
    *out_rgba = rgba;
    return 0;
}



/**
 * Capture the latest presented frame into a PNG file.
 *
 * @param canvas canvas to capture
 * @param path output png path
 * @returns 0 on success or a negative error code
 */
int dvz_canvas_capture_png(DvzCanvas* canvas, const char* path)
{
    ANN(canvas);
    ANN(path);

    uint32_t width = 0;
    uint32_t height = 0;
    uint8_t* rgba = NULL;
    if (dvz_canvas_capture_rgba(canvas, &width, &height, &rgba) != 0)
    {
        return -1;
    }

    int rc = dvz_write_png(path, width, height, rgba);
    dvz_free(rgba);
    return rc;
}



/**
 * Enable or disable the video sink on the canvas stream.
 *
 * @param canvas canvas associated with the stream
 * @param enable true to enable, false to disable
 * @param cfg optional sink configuration
 */
int dvz_canvas_configure_video_sink(DvzCanvas* canvas, bool enable, const DvzVideoSinkConfig* cfg)
{
    ANN(canvas);
    return dvz_canvas_stream_enable_video(canvas, enable, cfg);
}



/**
 * Enable or disable the live-image sink on the canvas stream.
 *
 * @param canvas canvas associated with the stream
 * @param enable true to enable, false to disable
 * @param cfg required sink configuration when enabling
 * @returns 0 on success or a negative sink error
 */
int dvz_canvas_configure_live_image_sink(
    DvzCanvas* canvas, bool enable, const DvzCanvasLiveImageSinkConfig* cfg)
{
    ANN(canvas);
    return dvz_canvas_stream_enable_live_image(canvas, enable, cfg);
}



/**
 * Return the underlying stream pointer.
 *
 * @param canvas target canvas
 */
DvzStream* dvz_canvas_stream(DvzCanvas* canvas)
{
    ANN(canvas);
    return canvas->stream;
}



/**
 * Return the recorded timings collected so far.
 *
 * @param canvas canvas handle
 * @param count optional destination for the number of samples
 */
const DvzFrameTiming* dvz_canvas_timings(const DvzCanvas* canvas, size_t* count)
{
    ANN(canvas);
    return dvz_canvas_timings_view(&canvas->timings, count);
}
