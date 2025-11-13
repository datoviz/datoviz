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
/*  Forward declarations */
/*************************************************************************************************/

typedef struct DvzDevice DvzDevice;
typedef struct MP4E_mux_tag MP4E_mux_t;
typedef struct mp4_h26x_writer_tag mp4_h26x_writer_t;
typedef struct DvzVideoBackend DvzVideoBackend;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

// Mux sample.
typedef struct
{
    uint64_t offset;
    uint32_t size;
    uint32_t duration;
} DvzMuxSample;



// Video encoder.
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

/**
 * Allocate and configure an encoder instance backed by the given device.
 *
 * @param device Vulkan device used for image/memory imports (may be NULL)
 * @param cfg optional encoder configuration, falls back to `dvz_video_encoder_default_config()`
 * @returns a new encoder handle or NULL on allocation failure
 */
DvzVideoEncoder* dvz_video_encoder_create(DvzDevice* device, const DvzVideoEncoderConfig* cfg);



/**
 * Start encoding using the current frame and synchronization details.
 *
 * @param enc encoder handle
 * @param image GPU image that contains the rendered frame
 * @param memory backing memory imported for the image
 * @param memory_size size of the imported memory
 * @param memory_fd optional DMA-BUF fd representing the memory (platform-specific)
 * @param wait_semaphore_fd timeline semaphore where sinks wait for the frame
 * @param bitstream_out optional FILE to write Annex B streams when post-muxing
 * @returns 0 on success or a negative error when setup or backend start fails
 */
int dvz_video_encoder_start(
    DvzVideoEncoder* enc, VkImage image, VkDeviceMemory memory, VkDeviceSize memory_size,
    int memory_fd, int wait_semaphore_fd, FILE* bitstream_out);



/**
 * Submit the encoded frame to the chosen backend and update timeline progress.
 *
 * @param enc encoder handle
 * @param wait_value timeline semaphore value to signal the encoder/writer
 * @returns 0 when submission succeeded or a negative error from the backend
 */
int dvz_video_encoder_submit(DvzVideoEncoder* enc, uint64_t wait_value);



/**
 * Stop the encoder and flush any pending output before destruction.
 *
 * @param enc encoder handle
 * @returns 0 on success or a negative backend error
 */
int dvz_video_encoder_stop(DvzVideoEncoder* enc);



/**
 * Destroy the encoder and release all associated resources.
 *
 * @param enc encoder handle (NULL-safe)
 */
void dvz_video_encoder_destroy(DvzVideoEncoder* enc);



/**
 * Compute the next sample duration in 90 kHz ticks for the current encoder config.
 *
 * @param enc encoder instance
 * @returns encoded sample duration or 0 when durations are unavailable
 */
uint32_t dvz_video_encoder_next_duration(DvzVideoEncoder* enc);



/**
 * Notify the encoder that a new encoded sample is available, for muxing or streaming.
 *
 * @param enc encoder instance
 * @param data pointer to the encoded bitstream
 * @param size buffer size in bytes
 * @param file_offset offset inside temporary bitstream when post muxing
 * @param duration sample duration in 90 kHz ticks
 * @param keyframe true when the sample is a keyframe
 */
void dvz_video_encoder_on_sample(
    DvzVideoEncoder* enc, const uint8_t* data, uint32_t size, uint64_t file_offset,
    uint32_t duration, bool keyframe);
