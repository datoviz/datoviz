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
#include "datoviz/window.h"
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
    DvzStream* stream;
    bool stream_started;
    bool swapchain_sink_attached;
    DvzCanvasFramePool frame_pool;
    DvzCanvasTimingState timings;
    DvzCanvasDraw draw_callback;
    void* draw_user_data;
    uint64_t frame_id;
    bool video_sink_enabled;
    bool supports_external_memory;
    bool supports_external_semaphore;
    DvzVma allocator;
    bool allocator_ready;
    DvzSemaphore timeline_semaphore;
    uint64_t timeline_value;
    bool timeline_ready;
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
    DvzCanvasTimingState* timings, uint64_t frame_id, double cpu_submit_us);

const DvzFrameTiming* dvz_canvas_timings_view(const DvzCanvasTimingState* timings, size_t* count);

void dvz_canvas_window_surface_refresh(DvzCanvas* canvas);

DvzCanvasSurfaceInfo dvz_canvas_window_surface_info(const DvzCanvas* canvas);

int dvz_canvas_stream_prepare(DvzCanvas* canvas);

int dvz_canvas_stream_start(DvzCanvas* canvas, const DvzStreamFrame* frame);

int dvz_canvas_stream_submit(DvzCanvas* canvas, uint64_t wait_value);

int dvz_canvas_stream_enable_video(
    DvzCanvas* canvas, bool enable, const DvzVideoSinkConfig* cfg);

const DvzStreamSinkBackend* dvz_canvas_swapchain_sink_backend(void);

int dvz_canvas_swapchain_init(DvzCanvas* canvas);

void dvz_canvas_swapchain_destroy(DvzCanvas* canvas);

int dvz_canvas_swapchain_acquire(DvzCanvas* canvas, DvzStreamFrame* frame);

int dvz_canvas_swapchain_present(DvzCanvas* canvas, uint64_t wait_value);

void dvz_canvas_swapchain_mark_out_of_date(DvzCanvas* canvas);

bool dvz_canvas_swapchain_handles_dirty(const DvzCanvas* canvas);

void dvz_canvas_swapchain_handles_refreshed(DvzCanvas* canvas);

VkExternalSemaphoreHandleTypeFlags dvz_canvas_timeline_handle_type(void);
