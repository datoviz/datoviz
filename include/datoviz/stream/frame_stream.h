/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Frame stream API                                                                             */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <volk.h>

#include "datoviz/common/macros.h"



/*************************************************************************************************/
/*  Forward declarations                                                                          */
/*************************************************************************************************/

typedef struct DvzDevice DvzDevice;
typedef struct DvzFrameStream DvzFrameStream;
typedef struct DvzFrameSinkBackend DvzFrameSinkBackend;
typedef struct DvzFrameSink DvzFrameSink;



/*************************************************************************************************/
/*  Types                                                                                        */
/*************************************************************************************************/

typedef struct
{
    uint32_t width;
    uint32_t height;
    uint32_t fps;
    VkFormat color_format;
} DvzFrameStreamConfig;

typedef struct
{
    VkImage image;
    VkDeviceMemory memory;
    VkDeviceSize memory_size;
    int memory_fd;
    int wait_semaphore_fd;
} DvzFrameStreamResources;

struct DvzFrameSink
{
    DvzFrameStream* stream;
    const DvzFrameSinkBackend* backend;
    void* backend_data;
    const void* config;
    bool started;
};

typedef struct
{
    const char* backend;
    const void* config;
} DvzFrameSinkRequest;

struct DvzFrameSinkBackend
{
    const char* name;
    bool (*probe)(const void* config);
    int (*create)(DvzFrameSink* sink, const void* config);
    int (*start)(DvzFrameSink* sink, const DvzFrameStreamResources* resources);
    int (*submit)(DvzFrameSink* sink, uint64_t timeline_value);
    int (*stop)(DvzFrameSink* sink);
    void (*destroy)(DvzFrameSink* sink);
};



/*************************************************************************************************/
/*  API                                                                                           */
/*************************************************************************************************/

EXTERN_C_ON

DVZ_EXPORT DvzFrameStreamConfig dvz_frame_stream_default_config(void);
DVZ_EXPORT DvzFrameStream*
dvz_frame_stream_create(DvzDevice* device, const DvzFrameStreamConfig* cfg);
DVZ_EXPORT void dvz_frame_stream_destroy(DvzFrameStream* stream);
DVZ_EXPORT int dvz_frame_stream_attach_sink(
    DvzFrameStream* stream, const DvzFrameSinkBackend* backend, const void* config);
DVZ_EXPORT int dvz_frame_stream_attach_sink_named(
    DvzFrameStream* stream, const char* backend_name, const void* config);
DVZ_EXPORT int dvz_frame_stream_start(
    DvzFrameStream* stream, const DvzFrameStreamResources* resources);
DVZ_EXPORT int dvz_frame_stream_submit(DvzFrameStream* stream, uint64_t wait_value);
DVZ_EXPORT int dvz_frame_stream_stop(DvzFrameStream* stream);
DVZ_EXPORT DvzDevice* dvz_frame_stream_device(DvzFrameStream* stream);
DVZ_EXPORT const DvzFrameStreamConfig* dvz_frame_stream_config(DvzFrameStream* stream);
DVZ_EXPORT const DvzFrameSinkBackend* dvz_frame_sink_backend_find(const char* name);
DVZ_EXPORT const DvzFrameSinkBackend*
dvz_frame_sink_backend_pick(const char* name, const void* config);

EXTERN_C_OFF

