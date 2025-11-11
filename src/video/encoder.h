/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Video encoder internals                                                                      */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stddef.h>

#include "datoviz/video.h"



/*************************************************************************************************/
/*  Forward declarations                                                                          */
/*************************************************************************************************/

typedef struct DvzDevice DvzDevice;
typedef struct MP4E_mux_tag MP4E_mux_t;
typedef struct mp4_h26x_writer_tag mp4_h26x_writer_t;
typedef struct DvzVideoBackend DvzVideoBackend;



/*************************************************************************************************/
/*  Types                                                                                        */
/*************************************************************************************************/

typedef struct
{
    uint64_t offset;
    uint32_t size;
    uint32_t duration;
} DvzMuxSample;

typedef struct DvzVideoEncoder
{
    DvzDevice* device;
    DvzVideoEncoderConfig cfg;
    bool started;
    uint32_t frame_idx;

    VkImage image;
    VkDeviceMemory memory;
    VkDeviceSize memory_size;
    int memory_fd;
    int wait_semaphore_fd;

    FILE* fp;
    bool own_bitstream_fp;
    DvzVideoMux mux;

    FILE* mp4_fp;
    MP4E_mux_t* mp4_mux;
    mp4_h26x_writer_t* mp4_writer;
    bool mp4_writer_ready;
    uint32_t mp4_frame_duration_num;
    uint32_t mp4_frame_duration_den;
    uint64_t mp4_frame_duration_accum;
    uint32_t mp4_frame_duration_last;
    char* mp4_path_owned;
    DvzMuxSample* mux_samples;
    size_t mux_sample_count;
    size_t mux_sample_capacity;

    const DvzVideoBackend* backend;
    void* backend_data;
} DvzVideoEncoder;



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

DvzVideoEncoderConfig dvz_video_encoder_default_config(void);
DvzVideoEncoder*
dvz_video_encoder_create(DvzDevice* device, const DvzVideoEncoderConfig* cfg);
int dvz_video_encoder_start(
    DvzVideoEncoder* enc,
    VkImage image,
    VkDeviceMemory memory,
    VkDeviceSize memory_size,
    int memory_fd,
    int wait_semaphore_fd,
    FILE* bitstream_out);
int dvz_video_encoder_submit(DvzVideoEncoder* enc, uint64_t wait_value);
int dvz_video_encoder_stop(DvzVideoEncoder* enc);
void dvz_video_encoder_destroy(DvzVideoEncoder* enc);
uint32_t dvz_video_encoder_next_duration(DvzVideoEncoder* enc);
void dvz_video_encoder_on_sample(
    DvzVideoEncoder* enc,
    const uint8_t* data,
    uint32_t size,
    uint64_t file_offset,
    uint32_t duration,
    bool keyframe);
