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

#include <volk.h>

#include "datoviz/common/macros.h"



/*************************************************************************************************/
/*  Forward declarations                                                                         */
/*************************************************************************************************/

typedef struct DvzDevice DvzDevice;
typedef struct DvzStream DvzStream;
typedef struct DvzStreamSinkBackend DvzStreamSinkBackend;
typedef struct DvzStreamSink DvzStreamSink;



/*************************************************************************************************/
/*  Types                                                                                        */
/*************************************************************************************************/

// Stream config.
typedef struct
{
    uint32_t width;
    uint32_t height;
    uint32_t fps;
    VkFormat color_format;
} DvzStreamConfig;



// Stream frame descriptor.
typedef struct
{
    VkImage image;
    VkDeviceMemory memory;
    VkDeviceSize memory_size;
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

