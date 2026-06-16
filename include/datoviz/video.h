/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Video                                                                                        */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include <volk.h>

#include "datoviz/common/macros.h"
#include "datoviz/stream.h"



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/

// VIdeo codec (H264, H265/HEVC).
typedef enum
{
    DVZ_VIDEO_CODEC_H264 = 0,
    DVZ_VIDEO_CODEC_HEVC = 1,
} DvzVideoCodec;



// Video mux strategy.
typedef enum
{
    DVZ_VIDEO_MUX_NONE = 0,          // no built-in mix, need to be done manually with e.g. ffmpeg
    DVZ_VIDEO_MUX_MP4_STREAMING = 1, // save to mp4 directly
    DVZ_VIDEO_MUX_MP4_POST = 2,      // save to temporary raw file and then mux with built-in muxer
} DvzVideoMux;



// Video capture strategy used by the sink.
typedef enum
{
    DVZ_VIDEO_CAPTURE_AUTO = 0,           // choose the best available path at runtime
    DVZ_VIDEO_CAPTURE_EXTERNAL = 1,       // use external memory/semaphore interop
    DVZ_VIDEO_CAPTURE_CPU_READBACK = 2,   // read back RGBA to CPU before encoding
} DvzVideoCaptureMode;



// CPU readback callback used by video sinks running in CPU capture mode. Returned pixels use the
// screenshot/export contract: tightly packed sRGB RGBA8 with straight linear alpha.
typedef int (*DvzVideoSinkCaptureFn)(
    void* user_data, uint32_t* out_width, uint32_t* out_height, size_t* out_stride,
    uint8_t** out_rgba);



// CPU readback release callback for buffers returned by DvzVideoSinkCaptureFn.
typedef void (*DvzVideoSinkReleaseFn)(void* user_data, uint8_t* rgba);



// Video encoder config.
typedef struct
{
    uint32_t struct_size;
    uint32_t flags;
    uint32_t width;
    uint32_t height;
    uint32_t fps;
    VkFormat color_format;
    DvzVideoCodec codec;
    DvzVideoMux mux;
    const char* mp4_path;
    const char* raw_path;
    const char* backend; // "auto", "nvenc", ...
} DvzVideoEncoderConfig;



// Video sink config.
typedef struct
{
    uint32_t struct_size;
    uint32_t flags;
    DvzVideoEncoderConfig encoder;
    FILE* bitstream;
    DvzVideoCaptureMode capture_mode;
    DvzVideoSinkCaptureFn capture_rgba;
    DvzVideoSinkReleaseFn release_rgba;
    void* capture_user_data;
} DvzVideoSinkConfig;



EXTERN_C_ON

/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Return the encoder configuration used when no overrides are supplied.
 *
 * @returns a config tuned for 1080p60 HEVC capture with MP4 streaming muxing
 */
DVZ_EXPORT DvzVideoEncoderConfig dvz_video_encoder_config(void);



/**
 * Return the default video sink configuration that wraps the encoder defaults.
 *
 * @returns a sink config whose encoder uses `dvz_video_encoder_config()` and null
 * bitstream
 */
DVZ_EXPORT DvzVideoSinkConfig dvz_video_sink_config(void);



/**
 * Access the built-in video sink backend that encodes stream frames.
 *
 * @returns the video sink backend descriptor (caller registers it in the target sink registry)
 */
DVZ_EXPORT const DvzStreamSinkBackend* dvz_stream_sink_video(void);



EXTERN_C_OFF
