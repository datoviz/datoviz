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

#include "../vk/macros.h"
#include "_alloc.h"
#include "_assertions.h"
#include "_log.h"
#include "_time_utils.h"
#include "datoviz/video.h"



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
        cfg.color_format = (canvas->cfg.color_format != VK_FORMAT_UNDEFINED)
                               ? canvas->cfg.color_format
                               : canvas->surface->format;
    }
    else if (canvas->cfg.color_format != VK_FORMAT_UNDEFINED)
    {
        cfg.color_format = canvas->cfg.color_format;
    }
    return cfg;
}


static const char* const CANVAS_REQUIRED_EXTENSIONS[] = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
// VK_KHR_TIMELINE_SEMAPHORE_EXTENSION_NAME,
// VK_KHR_EXTERNAL_MEMORY_EXTENSION_NAME,
// VK_KHR_EXTERNAL_SEMAPHORE_EXTENSION_NAME,
#if OS_UNIX
    VK_KHR_EXTERNAL_MEMORY_FD_EXTENSION_NAME,
    VK_KHR_EXTERNAL_SEMAPHORE_FD_EXTENSION_NAME,
#elif OS_WINDOWS
    VK_KHR_EXTERNAL_MEMORY_WIN32_EXTENSION_NAME,
    VK_KHR_EXTERNAL_SEMAPHORE_WIN32_EXTENSION_NAME,
#endif
};



static bool canvas_device_check_extensions(const DvzCanvas* canvas)
{
    ANN(canvas);
    ANN(canvas->device);
    const size_t required_count =
        sizeof(CANVAS_REQUIRED_EXTENSIONS) / sizeof(CANVAS_REQUIRED_EXTENSIONS[0]);
    for (size_t i = 0; i < required_count; ++i)
    {
        const char* name = CANVAS_REQUIRED_EXTENSIONS[i];
        if (!name || name[0] == '\0')
        {
            continue;
        }
        if (!dvz_device_has_extension(canvas->device, name))
        {
            log_error("canvas device missing required extension '%s'", name);
            return false;
        }
    }
    return true;
}



static VkExternalMemoryHandleTypeFlagsKHR canvas_external_memory_handle_type(void)
{
#if OS_UNIX
    return VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT;
#elif OS_WINDOWS
    return VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT;
#else
    return 0;
#endif
}



static VkExternalSemaphoreHandleTypeFlags canvas_external_semaphore_handle_type(void)
{
#if OS_UNIX
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
    VkExternalMemoryHandleTypeFlagsKHR handle_type = canvas_external_memory_handle_type();
    if (handle_type == 0)
    {
        log_error("platform does not support exporting canvas render targets");
        return -1;
    }
    if (dvz_device_allocator(canvas->device, handle_type, &canvas->allocator) != 0)
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
    dvz_allocator_destroy(&canvas->allocator);
    canvas->allocator_ready = false;
}



static int canvas_create_timeline(DvzCanvas* canvas)
{
    ANN(canvas);
    if (canvas->timeline_ready)
    {
        return 0;
    }
    VkExternalSemaphoreHandleTypeFlags handle_type = canvas_external_semaphore_handle_type();
    if (handle_type == 0)
    {
        log_error("platform does not support exporting canvas timeline semaphores");
        return -1;
    }

    VkDevice vk_device = dvz_device_handle(canvas->device);
    ANNVK(vk_device);

    VkSemaphoreTypeCreateInfo type_info = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO,
        .semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE,
        .initialValue = 0,
    };
    VkExportSemaphoreCreateInfo export_info = {
        .sType = VK_STRUCTURE_TYPE_EXPORT_SEMAPHORE_CREATE_INFO,
        .handleTypes = handle_type,
    };
    type_info.pNext = &export_info;
    VkSemaphoreCreateInfo info = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        .pNext = &type_info,
    };

    VkResult res = vkCreateSemaphore(vk_device, &info, NULL, &canvas->timeline_semaphore);
    if (res != VK_SUCCESS)
    {
        log_error("failed to create canvas timeline semaphore (%d)", res);
        canvas->timeline_semaphore = VK_NULL_HANDLE;
        return -1;
    }

#if OS_UNIX
    VkSemaphoreGetFdInfoKHR fd_info = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_GET_FD_INFO_KHR,
        .semaphore = canvas->timeline_semaphore,
        .handleType = handle_type,
    };
    res = vkGetSemaphoreFdKHR(vk_device, &fd_info, &canvas->timeline_semaphore_fd);
    if (res != VK_SUCCESS)
    {
        log_error("failed to export canvas timeline semaphore FD (%d)", res);
        vkDestroySemaphore(vk_device, canvas->timeline_semaphore, NULL);
        canvas->timeline_semaphore = VK_NULL_HANDLE;
        canvas->timeline_semaphore_fd = -1;
        return -1;
    }
#elif OS_WINDOWS
    log_error("timeline semaphore export not implemented on this platform");
    vkDestroySemaphore(vk_device, canvas->timeline_semaphore, NULL);
    canvas->timeline_semaphore = VK_NULL_HANDLE;
    return -1;
#else
    log_error("timeline semaphore export unsupported on this platform");
    vkDestroySemaphore(vk_device, canvas->timeline_semaphore, NULL);
    canvas->timeline_semaphore = VK_NULL_HANDLE;
    return -1;
#endif

    canvas->timeline_ready = true;
    canvas->timeline_value = 0;
    return 0;
}



static void canvas_destroy_timeline(DvzCanvas* canvas)
{
    if (!canvas || !canvas->timeline_ready)
    {
        return;
    }
    VkDevice vk_device = dvz_device_handle(canvas->device);
    if (canvas->timeline_semaphore != VK_NULL_HANDLE)
    {
        vkDestroySemaphore(vk_device, canvas->timeline_semaphore, NULL);
        canvas->timeline_semaphore = VK_NULL_HANDLE;
    }
#if OS_UNIX
    if (canvas->timeline_semaphore_fd >= 0)
    {
        close(canvas->timeline_semaphore_fd);
    }
#endif
    canvas->timeline_semaphore_fd = -1;
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
    pool->frames = (DvzStreamFrame*)calloc(pool->frame_count, sizeof(DvzStreamFrame));
    ANN(pool->frames);
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
        free(pool->frames);
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
        timings->samples = (DvzFrameTiming*)calloc(capacity, sizeof(DvzFrameTiming));
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
        free(timings->samples);
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
        .color_format = VK_FORMAT_B8G8R8A8_UNORM,
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

    DvzCanvas* canvas = (DvzCanvas*)calloc(1, sizeof(DvzCanvas));
    ANN(canvas);
    canvas->cfg = resolved;
    canvas->window = resolved.window;
    canvas->device = resolved.device;
    canvas->draw_callback = NULL;
    canvas->draw_user_data = NULL;
    canvas->frame_id = 0;
    canvas->video_sink_enabled = false;
    canvas->stream_started = false;
    canvas->swapchain_sink_attached = false;
    canvas->timeline_semaphore_fd = -1;

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
    DvzStreamConfig stream_cfg = canvas_stream_config(canvas);
    canvas->stream = dvz_stream_create(canvas->device, &stream_cfg);
    if (!canvas->stream)
    {
        log_error("failed to allocate stream for canvas");
        free(canvas);
        return NULL;
    }

    dvz_canvas_frame_pool_init(&canvas->frame_pool, 1);
    size_t timing_history =
        resolved.timing_history > 0 ? resolved.timing_history : DVZ_CANVAS_DEFAULT_TIMING_HISTORY;
    dvz_canvas_timings_init(&canvas->timings, timing_history);

    if (dvz_canvas_stream_prepare(canvas) != 0)
    {
        log_warn("canvas stream preparation failed; swapchain sink unavailable");
    }

    if (resolved.enable_video_sink)
    {
        dvz_canvas_configure_video_sink(canvas, true, NULL);
    }

    if (dvz_canvas_swapchain_init(canvas) != 0)
    {
        log_error("failed to initialize canvas swapchain state");
        dvz_canvas_destroy(canvas);
        return NULL;
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
    dvz_canvas_stream_enable_video(canvas, false, NULL);
    if (canvas->stream)
    {
        dvz_stream_destroy(canvas->stream);
        canvas->stream = NULL;
    }
    canvas_destroy_timeline(canvas);
    canvas_destroy_allocator(canvas);
    dvz_canvas_frame_pool_release(&canvas->frame_pool);
    dvz_canvas_timings_release(&canvas->timings);
    free(canvas);
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

    DvzStreamFrame* frame = dvz_canvas_frame_pool_rotate(&canvas->frame_pool);
    if (!frame)
    {
        log_error("canvas frame pool unavailable");
        return -1;
    }

    int acquire_rc = dvz_canvas_swapchain_acquire(canvas, frame);
    if (acquire_rc == DVZ_CANVAS_FRAME_WAIT_SURFACE)
    {
        return DVZ_CANVAS_FRAME_WAIT_SURFACE;
    }
    if (acquire_rc != 0)
    {
        log_warn("unable to acquire canvas frame from swapchain");
        return -1;
    }

    bool stream_was_started = canvas->stream_started;
    if (dvz_canvas_stream_start(canvas, frame) != 0)
    {
        return -1;
    }
    bool stream_started_now = !stream_was_started && canvas->stream_started;
    if (stream_started_now)
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
    int result = dvz_canvas_stream_submit(canvas, wait_value);
    if (result == 0)
    {
        canvas->timeline_value = wait_value;
    }
    double elapsed = dvz_clock_interval(&clock) * 1e6;
    dvz_canvas_timings_record(&canvas->timings, canvas->frame_id, elapsed);
    return result;
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
