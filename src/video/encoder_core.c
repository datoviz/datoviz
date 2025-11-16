/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Video                                                                                        */
/*************************************************************************************************/

#include "encoder.h"
#include "encoder_backend.h"
#include "encoder_internal.h"

#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

#include "_alloc.h"
#include "_log.h"
#include "datoviz/common/macros.h"
#include "datoviz/vk/enums.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

// ====== Params ======
// Video settings cheat sheet:
// - Prefer HEVC/H.265 when hardware encoders exist (NVENC, VideoToolbox, VA-API). Fall back to
// H.264
//   High profile for max compatibility or use kvazaar/x264 as CPU software encoders.
// - 1080p60 interactive captures: target 12-18 Mb/s (CQP 20/22/24 or CRF 18-20), GOP length = 2s,
// 2
//   B-frames. Expects ~90-135 MB per minute once muxed into MP4.
// - Social media masters (H.264 High profile):
//     * 1080p30: 8-10 Mb/s
//     * 1080p60: 12-15 Mb/s
//     * 4K30: 25-35 Mb/s (15-20 Mb/s if HEVC is allowed)
//     * Vertical 1080x1920@30: 6-8 Mb/s
// - Platform backends:
//     * Linux/Windows + NVIDIA: CUDA external memory + NVENC.
//     * macOS: MoltenVK-exported IOSurface → Metal blit/compute → VideoToolbox.
//     * Linux Intel/AMD: exportable VkImage → VA-API/AMF.
//     * CPU fallback: convert to planar YUV420 and feed kvazaar (BSD) or x264/x265 (GPL).
// - Audio/mux later; this harness simply emits Annex B bitstreams (out.h265).

#define WIDTH  1920
#define HEIGHT 1080
#define FPS    60



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

static void video_encoder_release(DvzVideoEncoder* enc)
{
    if (!enc)
    {
        return;
    }

    if (enc->backend && enc->backend->destroy)
    {
        enc->backend->destroy(enc);
        enc->backend_data = NULL;
    }

    dvz_video_encoder_close_mp4(enc);
    if (enc->mp4_path_owned)
    {
        dvz_free(enc->mp4_path_owned);
        enc->mp4_path_owned = NULL;
    }
    dvz_free(enc->mux_samples);
    enc->mux_samples = NULL;
    enc->mux_sample_capacity = 0;
    enc->mux_sample_count = 0;

    if (enc->own_bitstream_fp && enc->fp)
    {
        fclose(enc->fp);
    }
    enc->own_bitstream_fp = false;
    enc->fp = NULL;
    enc->image = VK_NULL_HANDLE;
    enc->memory = VK_NULL_HANDLE;
    enc->memory_size = 0;
    enc->memory_fd = -1;
    enc->wait_semaphore_fd = -1;
    enc->started = false;
    enc->frame_idx = 0;
}



/*************************************************************************************************/
/*  Front-end API                                                                                */
/*************************************************************************************************/

DvzVideoEncoderConfig dvz_video_encoder_default_config(void)
{
    DvzVideoEncoderConfig cfg = {
        .width = WIDTH,
        .height = HEIGHT,
        .fps = FPS,
        .color_format = DVZ_DEFAULT_COLOR_FORMAT,
        .codec = DVZ_VIDEO_CODEC_HEVC,
        .mux = DVZ_VIDEO_MUX_MP4_STREAMING,
        .mp4_path = "out.mp4",
        .raw_path = "out.h26x",
        .backend = "auto",
        .flags = 0,
    };
    return cfg;
}



DvzVideoEncoder* dvz_video_encoder_create(DvzDevice* device, const DvzVideoEncoderConfig* cfg)
{
    DvzVideoEncoder* enc = (DvzVideoEncoder*)dvz_calloc(1, sizeof(DvzVideoEncoder));
    ANN(enc);
    enc->device = device;
    enc->cfg = cfg ? *cfg : dvz_video_encoder_default_config();
    enc->memory_fd = -1;
    enc->wait_semaphore_fd = -1;
    enc->mp4_writer = NULL;

    enc->backend = dvz_video_backend_pick(&enc->cfg);
    if (!enc->backend || !enc->backend->init || enc->backend->init(enc) != 0)
    {
        log_error("failed to initialize video backend");
        dvz_free(enc);
        return NULL;
    }

    return enc;
}



int dvz_video_encoder_start(
    DvzVideoEncoder* enc, VkImage image, VkDeviceMemory memory, VkDeviceSize memory_size,
    int memory_fd, int wait_semaphore_fd, FILE* bitstream_out)
{
    ANN(enc);
    if (enc->started)
    {
        log_error("video encoder already started");
        return -1;
    }
    if (!enc->backend || !enc->backend->start)
    {
        log_error("video encoder backend missing start()");
        return -1;
    }

    if (!enc->backend_data && enc->backend->init)
    {
        if (enc->backend->init(enc) != 0)
        {
            log_error("video backend init failed");
            return -1;
        }
    }

    int rc = 0;
    enc->mux = enc->cfg.mux;
    enc->mp4_frame_duration_num = 90000;
    enc->mp4_frame_duration_den = enc->cfg.fps > 0 ? enc->cfg.fps : 1;
    enc->mp4_frame_duration_accum = 0;
    enc->image = image;
    enc->memory = memory;
    enc->memory_size = memory_size;
    enc->memory_fd = memory_fd;
    enc->wait_semaphore_fd = wait_semaphore_fd;

    if (enc->mux == DVZ_VIDEO_MUX_MP4_STREAMING)
    {
        if (!dvz_video_encoder_open_mp4_stream(enc))
        {
            rc = -1;
            goto fail;
        }
    }

    if (!bitstream_out && enc->mux == DVZ_VIDEO_MUX_MP4_POST)
    {
        const char* raw_path = enc->cfg.raw_path ? enc->cfg.raw_path : "out.h26x";
        bitstream_out = fopen(raw_path, "w+b"); // MP4 post-processing rereads this stream.
        if (!bitstream_out)
        {
            log_error("failed to open raw bitstream '%s': %s", raw_path, strerror(errno));
            rc = -1;
            goto fail;
        }
        enc->own_bitstream_fp = true;
    }
    enc->fp = bitstream_out;

    if (enc->backend->start(enc) != 0)
    {
        rc = -1;
        goto fail;
    }

    enc->frame_idx = 0;
    enc->started = true;
    return 0;

fail:
    video_encoder_release(enc);
    return rc;
}



int dvz_video_encoder_submit(DvzVideoEncoder* enc, uint64_t wait_value)
{
    ANN(enc);
    if (!enc->started || !enc->backend || !enc->backend->submit)
    {
        return -1;
    }
    int rc = enc->backend->submit(enc, wait_value);
    if (rc == 0)
    {
        enc->frame_idx += 1;
    }
    return rc;
}



int dvz_video_encoder_stop(DvzVideoEncoder* enc)
{
    if (!enc)
    {
        return 0;
    }

    if (enc->started && enc->backend && enc->backend->stop)
    {
        enc->backend->stop(enc);
    }
    if (enc->mux == DVZ_VIDEO_MUX_MP4_POST && enc->fp)
    {
        fflush(enc->fp);
        dvz_video_encoder_mux_post(enc);
        fseeko(enc->fp, 0, SEEK_END);
    }
    video_encoder_release(enc);
    return 0;
}



void dvz_video_encoder_destroy(DvzVideoEncoder* enc)
{
    if (!enc)
    {
        return;
    }
    dvz_video_encoder_stop(enc);
    if (enc->mp4_writer)
    {
        dvz_free(enc->mp4_writer);
        enc->mp4_writer = NULL;
    }
    dvz_free(enc);
}



/*************************************************************************************************/
/*  Backend helpers                                                                              */
/*************************************************************************************************/

void dvz_video_encoder_on_sample(
    DvzVideoEncoder* enc, const uint8_t* data, uint32_t size, uint64_t file_offset,
    uint32_t duration, bool keyframe)
{
    (void)keyframe;
    if (!enc || size == 0)
    {
        return;
    }
    dvz_video_encoder_mux_sample(enc, data, size, duration);
    if (file_offset != UINT64_MAX)
    {
        dvz_video_encoder_record_sample(enc, file_offset, size, duration);
    }
}



uint32_t dvz_video_encoder_next_duration(DvzVideoEncoder* enc)
{
    if (!enc || enc->mp4_frame_duration_den == 0)
    {
        return 0;
    }
    enc->mp4_frame_duration_accum += enc->mp4_frame_duration_num;
    uint32_t duration = (uint32_t)(enc->mp4_frame_duration_accum / enc->mp4_frame_duration_den);
    enc->mp4_frame_duration_accum -= (uint64_t)duration * (uint64_t)enc->mp4_frame_duration_den;
    enc->mp4_frame_duration_last = duration;
    return duration;
}
