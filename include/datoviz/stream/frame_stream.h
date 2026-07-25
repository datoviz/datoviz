/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Frame stream types                                                                           */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "datoviz/vk/vulkan.h"
#include <volk.h>

#include "datoviz/vk/enums.h"
#include "datoviz/common/macros.h"



/*************************************************************************************************/
/*  Forward declarations                                                                         */
/*************************************************************************************************/

typedef struct DvzDevice DvzDevice;
typedef struct DvzStream DvzStream;
typedef struct DvzStreamSinkBackend DvzStreamSinkBackend;
typedef struct DvzStreamSink DvzStreamSink;
typedef struct DvzStreamSinkRegistry DvzStreamSinkRegistry;



/*************************************************************************************************/
/*  Types                                                                                        */
/*************************************************************************************************/

// Stream config.
typedef struct
{
    uint32_t struct_size;
    uint32_t flags;
    uint32_t width;
    uint32_t height;
    uint32_t fps;
    VkFormat color_format;
} DvzStreamConfig;



// Stream frame usage flags.
typedef enum
{
    DVZ_STREAM_FRAME_USAGE_NONE = 0,
    DVZ_STREAM_FRAME_USAGE_RENDER_TARGET = 1u << 0,
    DVZ_STREAM_FRAME_USAGE_COPY_SRC = 1u << 1,
    DVZ_STREAM_FRAME_USAGE_COPY_DST = 1u << 2,
} DvzStreamFrameUsage;



// Stream frame descriptor.
//
// Vulkan handles are owned by the stream unless the corresponding `*_borrowed` field is true. A
// borrowed handle is never destroyed by the receiver. A stream sink may inspect handles during its
// start/update/submit callbacks, but must not destroy, reset, transition, or retain them unless the
// sink duplicates the underlying OS handle or owns the wrapped object by contract. A configured
// depth attachment is already in `depth_layout` when the recording callback begins and is part of
// the resource set identified by `resource_generation`; the receiver may use it only as an
// attachment in that reported layout. `memory_fd` and `wait_semaphore_fd` are callback-duration
// descriptors owned by Datoviz; duplicate before retaining and do not close the originals.
// `command_buffer_recording` tells sinks whether the command buffer is currently open for recording.
typedef struct DvzStreamFrame
{
    VkImage image;
    VkDeviceMemory memory;
    VkDeviceSize memory_size;
    VkCommandBuffer command_buffer;
    VkImageView image_view;
    VkExtent2D extent;
    VkFormat color_format;
    VkImageLayout image_layout;
    VkImage depth_image;
    VkImageView depth_view;
    VkFormat depth_format;
    VkImageLayout depth_layout;
    uint32_t usage;
    bool command_buffer_recording;
    bool image_borrowed;
    bool image_view_borrowed;
    bool command_buffer_borrowed;
    bool depth_image_borrowed;
    bool depth_view_borrowed;
    bool handles_dirty;
    uint64_t resource_generation;
    bool image_valid;
    bool depth_valid;
    int memory_fd;
    int wait_semaphore_fd;
} DvzStreamFrame;



// Stream sink instance.
struct DvzStreamSink
{
    DvzStream* stream;
    const DvzStreamSinkBackend* backend;
    void* backend_data;
    const void* config;
    bool started;
};



// Stream sink request resolved through the registry.
typedef struct
{
    const char* backend;
    const void* config;
} DvzStreamSinkRequest;



// Stream sink backend descriptor.
struct DvzStreamSinkBackend
{
    const char* name;
    bool (*probe)(const void* config);
    int (*create)(DvzStreamSink* sink, const void* config);
    int (*start)(DvzStreamSink* sink, const DvzStreamFrame* frame);
    int (*submit)(DvzStreamSink* sink, uint64_t timeline_value);
    int (*stop)(DvzStreamSink* sink);
    int (*update)(DvzStreamSink* sink, const DvzStreamFrame* frame);
    void (*destroy)(DvzStreamSink* sink);
};
