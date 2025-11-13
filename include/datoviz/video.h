/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Video                                                                                       */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include <volk.h>

#include "datoviz/common/macros.h"
#include "datoviz/stream/frame_stream.h"



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/



/*************************************************************************************************/
typedef enum
{
    DVZ_VIDEO_CODEC_H264 = 0,
    DVZ_VIDEO_CODEC_HEVC = 1,
} DvzVideoCodec;

typedef enum
{
    DVZ_VIDEO_MUX_NONE = 0,
    DVZ_VIDEO_MUX_MP4_STREAMING = 1,
    DVZ_VIDEO_MUX_MP4_POST = 2,
} DvzVideoMux;

typedef struct
{
    uint32_t width;
    uint32_t height;
    uint32_t fps;
    VkFormat color_format;
    DvzVideoCodec codec;
    DvzVideoMux mux;
    const char* mp4_path;
    const char* raw_path;
    const char* backend; // \"auto\", \"nvenc\", ...
    int flags;
} DvzVideoEncoderConfig;

typedef struct
{
    DvzVideoEncoderConfig encoder;
    FILE* bitstream;
} DvzVideoSinkConfig;



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

EXTERN_C_ON


DVZ_EXPORT DvzVideoEncoderConfig dvz_video_encoder_default_config(void);
DVZ_EXPORT DvzVideoSinkConfig dvz_video_sink_default_config(void);
DVZ_EXPORT const DvzStreamSinkBackend* dvz_stream_sink_video(void);



EXTERN_C_OFF
