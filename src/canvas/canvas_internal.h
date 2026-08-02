/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Canvas internals                                                                             */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <volk.h>

#include "datoviz/canvas.h"
#include "datoviz/common/macros.h"
#include "datoviz/stream.h"
#include "datoviz/vk/device.h"
#include "datoviz/vk/memory.h"
#include "datoviz/vk/memory_interop.h"
#include "datoviz/window.h"
#include "datoviz/vklite/images.h"
#include "datoviz/vklite/sync.h"



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzCanvasFramePool DvzCanvasFramePool;
typedef struct DvzCanvasTimingState DvzCanvasTimingState;
typedef struct DvzCanvasSurfaceInfo DvzCanvasSurfaceInfo;
typedef struct DvzCanvasSwapchain DvzCanvasSwapchain;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzCanvasFramePool
{
    DvzStreamFrame* frames;
    uint32_t frame_count;
    uint32_t current_index;
};



struct DvzCanvasTimingState
{
    DvzFrameTiming* samples;
    size_t capacity;
    size_t count;
    size_t head;
};



struct DvzCanvas
{
    DvzCanvasConfig cfg;
    DvzWindow* window;
    const DvzWindowSurface* surface;
    DvzDevice* device;
    DvzStreamSinkRegistry* sink_registry;
    DvzStream* stream;
    bool stream_started;
    uint64_t stream_resource_generation;
    bool primary_sink_attached;
    DvzCanvasFramePool frame_pool;
    DvzCanvasTimingState timings;
    double current_slot_wait_us;
    double current_acquire_wait_us;
    DvzCanvasDraw draw_callback;
    void* draw_user_data;
    uint64_t frame_id;
    bool video_sink_enabled;
    DvzVideoCaptureMode video_capture_mode;
    DvzVideoSinkConfig video_sink_cfg;
    bool video_sink_cfg_valid;
    bool live_image_sink_enabled;
    DvzCanvasLiveImageSinkConfig live_image_sink_cfg;
    bool supports_external_memory;
    bool supports_external_semaphore;
    DvzVma* allocator;
    DvzVma* readback_allocator;
    bool allocator_ready;
    DvzSemaphore* timeline_semaphore;
    uint64_t timeline_value;
    bool timeline_ready;
    bool test_force_wait_semaphore_export_failure;
    int32_t test_force_offscreen_submit_status;
    bool test_force_offscreen_submit_status_set;
    VkImage offscreen_image;
    VkImageView offscreen_view;
    DvzAllocation* offscreen_alloc;
    DvzImages* offscreen_images;
    DvzImageViews* offscreen_views;
    DvzImages* offscreen_depth_images;
    DvzImageViews* offscreen_depth_views;
    VkImage offscreen_depth_image;
    VkImageView offscreen_depth_view;
    VkImageLayout offscreen_depth_layout;
    VkImageLayout offscreen_layout;
    VkCommandBuffer offscreen_command_buffer;
    VkImage retired_offscreen_image;
    VkImageView retired_offscreen_view;
    DvzAllocation* retired_offscreen_alloc;
    DvzImages* retired_offscreen_images;
    DvzImageViews* retired_offscreen_views;
    DvzImages* retired_offscreen_depth_images;
    DvzImageViews* retired_offscreen_depth_views;
    VkImage retired_offscreen_depth_image;
    VkImageView retired_offscreen_depth_view;
    VkCommandBuffer retired_offscreen_command_buffer;
    DvzExternalHandle retired_offscreen_memory_fd;
    VkQueue offscreen_queue;
    uint32_t offscreen_queue_family;
    VkExtent2D offscreen_extent;
    VkFormat offscreen_format;
    uint64_t offscreen_resource_generation;
    DvzExternalHandle offscreen_memory_fd;
    bool offscreen_ready;
    DvzCanvasOffscreenRuntimeState offscreen_runtime_state;
    DvzCanvasSwapchain* swapchain;
};



/*************************************************************************************************/
/*  Surface info                                                                                 */
/*************************************************************************************************/

struct DvzCanvasSurfaceInfo
{
    VkExtent2D extent;
    VkFormat format;
    float scale_x;
    float scale_y;
};



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

void dvz_canvas_frame_pool_init(DvzCanvasFramePool* pool, uint32_t frame_count);

void dvz_canvas_frame_pool_release(DvzCanvasFramePool* pool);

DvzStreamFrame* dvz_canvas_frame_pool_current(DvzCanvasFramePool* pool);

DvzStreamFrame* dvz_canvas_frame_pool_rotate(DvzCanvasFramePool* pool);

void dvz_canvas_timings_init(DvzCanvasTimingState* timings, size_t capacity);

void dvz_canvas_timings_release(DvzCanvasTimingState* timings);

void dvz_canvas_timings_record(
    DvzCanvasTimingState* timings, uint64_t frame_id, double cpu_submit_us, double slot_wait_us,
    double acquire_wait_us);

const DvzFrameTiming* dvz_canvas_timings_view(const DvzCanvasTimingState* timings, size_t* count);

void dvz_canvas_window_surface_refresh(DvzCanvas* canvas);

DvzCanvasSurfaceInfo dvz_canvas_window_surface_info(const DvzCanvas* canvas);

int dvz_canvas_stream_prepare(DvzCanvas* canvas);

int dvz_canvas_stream_start(DvzCanvas* canvas, const DvzStreamFrame* frame);

int dvz_canvas_stream_submit(DvzCanvas* canvas, uint64_t wait_value);

int dvz_canvas_stream_enable_video(
    DvzCanvas* canvas, bool enable, const DvzVideoSinkConfig* cfg);

bool dvz_canvas_live_image_sink_config_validate(
    const DvzCanvasLiveImageSinkConfig* cfg);

int dvz_canvas_stream_enable_live_image(
    DvzCanvas* canvas, bool enable, const DvzCanvasLiveImageSinkConfig* cfg);

const DvzStreamSinkBackend* dvz_canvas_swapchain_sink_backend(void);

const DvzStreamSinkBackend* dvz_canvas_offscreen_sink_backend(void);

const DvzStreamSinkBackend* dvz_canvas_live_image_sink_backend(void);

int dvz_canvas_swapchain_init(DvzCanvas* canvas);

void dvz_canvas_swapchain_destroy(DvzCanvas* canvas);

int dvz_canvas_swapchain_acquire(DvzCanvas* canvas, DvzStreamFrame* frame);

int dvz_canvas_swapchain_present(DvzCanvas* canvas, uint64_t wait_value);

bool dvz_canvas_depth_format_valid(VkFormat format);

VkImageAspectFlags dvz_canvas_depth_aspect(VkFormat format);

VkImageLayout dvz_canvas_depth_layout(VkFormat format);

void dvz_canvas_cmd_clear_new_target(
    DvzCanvas* canvas, VkCommandBuffer cmd, VkImageView view, VkExtent2D extent,
    VkImageView depth_view, VkFormat depth_format);

int dvz_canvas_depth_create(
    DvzCanvas* canvas, VkExtent2D extent, DvzImages** images, DvzImageViews** views,
    VkImage* image, VkImageView* view);

void dvz_canvas_depth_destroy(
    DvzImages** images, DvzImageViews** views, VkImage* image, VkImageView* view);

void dvz_canvas_swapchain_mark_out_of_date(DvzCanvas* canvas);

bool _dvz_canvas_max_frames_in_flight_parse(const char* value, uint32_t* requested_slot_count);

uint32_t _dvz_canvas_frame_slot_count_resolve(
    uint32_t requested_slot_count, uint32_t image_count);

uint32_t _dvz_canvas_swapchain_image_count(const DvzCanvas* canvas);

uint32_t _dvz_canvas_swapchain_slot_count(const DvzCanvas* canvas);

uint32_t _dvz_canvas_swapchain_last_presented_slot_index(const DvzCanvas* canvas);

uint32_t _dvz_canvas_swapchain_last_presented_image_index(const DvzCanvas* canvas);

bool dvz_canvas_swapchain_present_mode(const DvzCanvas* canvas, VkPresentModeKHR* out_mode);

VkFormat dvz_canvas_swapchain_frame_format(const DvzCanvas* canvas);

void dvz_canvas_swapchain_handles_refreshed(DvzCanvas* canvas);

int dvz_canvas_swapchain_capture_rgba_into(
    DvzCanvas* canvas, uint32_t width, uint32_t height, uint8_t* out_rgba,
    DvzSize out_size_bytes);

void dvz_canvas_swapchain_test_fail_slot(DvzCanvas* canvas, int32_t slot_index);

void dvz_canvas_swapchain_test_force_recreate_status(DvzCanvas* canvas, int32_t status);

void dvz_canvas_swapchain_test_force_acquire_status(DvzCanvas* canvas, int32_t status);

void dvz_canvas_swapchain_test_force_present_status(DvzCanvas* canvas, int32_t status);

DvzCanvasPresentRuntimeState dvz_canvas_swapchain_runtime_state(const DvzCanvas* canvas);

uint64_t dvz_canvas_swapchain_recreate_count(const DvzCanvas* canvas);

VkExternalSemaphoreHandleTypeFlags dvz_canvas_timeline_handle_type(void);

void dvz_canvas_test_force_wait_semaphore_export_failure(DvzCanvas* canvas, bool enabled);

void dvz_canvas_test_force_offscreen_submit_status(DvzCanvas* canvas, int32_t status);
