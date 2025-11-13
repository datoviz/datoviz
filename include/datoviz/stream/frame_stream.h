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
typedef struct DvzStream DvzStream;
typedef struct DvzStreamSinkBackend DvzStreamSinkBackend;
typedef struct DvzStreamSink DvzStreamSink;



/*************************************************************************************************/
/*  Types                                                                                        */
/*************************************************************************************************/

typedef struct
{
    uint32_t width;
    uint32_t height;
    uint32_t fps;
    VkFormat color_format;
} DvzStreamConfig;

typedef struct
{
    VkImage image;
    VkDeviceMemory memory;
    VkDeviceSize memory_size;
    int memory_fd;
    int wait_semaphore_fd;
} DvzStreamFrame;

struct DvzStreamSink
{
    DvzStream* stream;
    const DvzStreamSinkBackend* backend;
    void* backend_data;
    const void* config;
    bool started;
};

typedef struct
{
    const char* backend;
    const void* config;
} DvzStreamSinkRequest;

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



/*************************************************************************************************/
/*  API                                                                                           */
/*************************************************************************************************/

EXTERN_C_ON

DVZ_EXPORT DvzStreamConfig dvz_stream_default_config(void);
DVZ_EXPORT DvzStream* dvz_stream_create(DvzDevice* device, const DvzStreamConfig* cfg);
DVZ_EXPORT void dvz_stream_destroy(DvzStream* stream);
DVZ_EXPORT int dvz_stream_attach_sink(
    DvzStream* stream, const DvzStreamSinkBackend* backend, const void* config);
DVZ_EXPORT int dvz_stream_attach_sink_name(
    DvzStream* stream, const char* backend_name, const void* config);
DVZ_EXPORT int dvz_stream_start(DvzStream* stream, const DvzStreamFrame* frame);
DVZ_EXPORT int dvz_stream_submit(DvzStream* stream, uint64_t wait_value);
DVZ_EXPORT int dvz_stream_update(DvzStream* stream, const DvzStreamFrame* frame);
DVZ_EXPORT int dvz_stream_stop(DvzStream* stream);
DVZ_EXPORT DvzDevice* dvz_stream_device(DvzStream* stream);
DVZ_EXPORT const DvzStreamConfig* dvz_stream_config(DvzStream* stream);
DVZ_EXPORT void dvz_stream_register_sink(const DvzStreamSinkBackend* backend);
DVZ_EXPORT const DvzStreamSinkBackend* dvz_stream_sink_find(const char* name);
DVZ_EXPORT const DvzStreamSinkBackend* dvz_stream_sink_pick(const char* name, const void* config);

EXTERN_C_OFF
