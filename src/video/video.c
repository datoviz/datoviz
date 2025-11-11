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

#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

#define MINIMP4_IMPLEMENTATION
#if defined(__clang__)
#    pragma clang diagnostic push
#    pragma clang diagnostic ignored "-Weverything"
#elif defined(__GNUC__)
#    pragma GCC diagnostic push
#    pragma GCC diagnostic ignored "-Wall"
#    pragma GCC diagnostic ignored "-Wextra"
#    pragma GCC diagnostic ignored "-Wpedantic"
#endif
#include "../../external/minimp4.h"
#if defined(__clang__)
#    pragma clang diagnostic pop
#elif defined(__GNUC__)
#    pragma GCC diagnostic pop
#endif

#include "_alloc.h"
#include "_log.h"
#include "datoviz/common/macros.h"



/*************************************************************************************************/
/*  NVENC video encode                                                                           */
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
#define WIDTH   1920
#define HEIGHT  1080
#define FPS     60
#define SECONDS 60



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

static uint32_t dvz_video_encoder_next_duration_internal(DvzVideoEncoder* enc);
static void dvz_video_encoder_mux_sample(
    DvzVideoEncoder* enc, const uint8_t* data, uint32_t size, uint32_t duration);
static void dvz_video_encoder_record_sample(
    DvzVideoEncoder* enc, uint64_t offset, uint32_t size, uint32_t duration);
static void dvz_video_encoder_close_mp4(DvzVideoEncoder* enc);
static bool dvz_video_encoder_open_mp4_stream(DvzVideoEncoder* enc);
static int dvz_video_encoder_mux_post(DvzVideoEncoder* enc);
static void dvz_video_encoder_release(DvzVideoEncoder* enc);

static int dvz_mp4_write_cb(int64_t offset, const void* buffer, size_t size, void* token)
{
    FILE* fp = (FILE*)token;
    if (!fp)
    {
        return MP4E_STATUS_FILE_WRITE_ERROR;
    }
    if (ftello(fp) != offset)
    {
        if (fseeko(fp, offset, SEEK_SET) != 0)
        {
            return MP4E_STATUS_FILE_WRITE_ERROR;
        }
    }
    size_t written = fwrite(buffer, 1, size, fp);
    return (written == size) ? MP4E_STATUS_OK : MP4E_STATUS_FILE_WRITE_ERROR;
}

static uint32_t dvz_video_encoder_next_duration_internal(DvzVideoEncoder* enc)
{
    if (!enc || enc->mp4_frame_duration_den == 0)
    {
        return 0;
    }
    enc->mp4_frame_duration_accum += enc->mp4_frame_duration_num;
    uint32_t duration = (uint32_t)(enc->mp4_frame_duration_accum / enc->mp4_frame_duration_den);
    enc->mp4_frame_duration_accum -=
        (uint64_t)duration * (uint64_t)enc->mp4_frame_duration_den;
    enc->mp4_frame_duration_last = duration;
    return duration;
}

uint32_t dvz_video_encoder_next_duration(DvzVideoEncoder* enc)
{
    return dvz_video_encoder_next_duration_internal(enc);
}

static void dvz_video_encoder_mux_sample(
    DvzVideoEncoder* enc, const uint8_t* data, uint32_t size, uint32_t duration)
{
    if (!enc || enc->mux != DVZ_VIDEO_MUX_MP4_STREAMING || !enc->mp4_writer_ready)
    {
        return;
    }
    if (data == NULL || size == 0 || !enc->mp4_writer)
    {
        return;
    }
    int err = mp4_h26x_write_nal(enc->mp4_writer, data, (int)size, duration);
    if (err != MP4E_STATUS_OK)
    {
        log_error("minimp4 streaming failed with status %d", err);
        mp4_h26x_write_close(enc->mp4_writer);
        MP4E_close(enc->mp4_mux);
        enc->mp4_writer_ready = false;
        enc->mp4_mux = NULL;
        if (enc->mp4_fp)
        {
            fclose(enc->mp4_fp);
            enc->mp4_fp = NULL;
        }
    }
}

static void dvz_video_encoder_record_sample(
    DvzVideoEncoder* enc, uint64_t offset, uint32_t size, uint32_t duration)
{
    if (!enc || enc->mux != DVZ_VIDEO_MUX_MP4_POST)
    {
        return;
    }
    if (enc->mux_sample_count == enc->mux_sample_capacity)
    {
        size_t new_cap = enc->mux_sample_capacity == 0 ? 128 : enc->mux_sample_capacity * 2;
        DvzMuxSample* samples =
            (DvzMuxSample*)realloc(enc->mux_samples, new_cap * sizeof(DvzMuxSample));
        if (!samples)
        {
            log_error("failed to grow mux sample buffer");
            return;
        }
        enc->mux_samples = samples;
        enc->mux_sample_capacity = new_cap;
    }
    enc->mux_samples[enc->mux_sample_count++] =
        (DvzMuxSample){.offset = offset, .size = size, .duration = duration};
}

static void dvz_video_encoder_close_mp4(DvzVideoEncoder* enc)
{
    if (!enc)
    {
        return;
    }
    if (enc->mp4_writer_ready && enc->mp4_writer)
    {
        mp4_h26x_write_close(enc->mp4_writer);
        enc->mp4_writer_ready = false;
    }
    if (enc->mp4_mux)
    {
        MP4E_close(enc->mp4_mux);
        enc->mp4_mux = NULL;
    }
    if (enc->mp4_fp)
    {
        fclose(enc->mp4_fp);
        enc->mp4_fp = NULL;
    }
}

static bool dvz_video_encoder_open_mp4_stream(DvzVideoEncoder* enc)
{
    ANN(enc);
    const char* mp4_path = enc->cfg.mp4_path ? enc->cfg.mp4_path : "out.mp4";
    enc->mp4_path_owned = dvz_strdup(mp4_path);
    enc->mp4_fp = fopen(mp4_path, "wb");
    if (!enc->mp4_fp)
    {
        log_error("failed to open mp4 output '%s': %s", mp4_path, strerror(errno));
        return false;
    }
    enc->mp4_mux = MP4E_open(1, 0, enc->mp4_fp, dvz_mp4_write_cb);
    if (!enc->mp4_mux)
    {
        log_error("failed to initialize minimp4 muxer");
        return false;
    }
    if (!enc->mp4_writer)
    {
        enc->mp4_writer = (mp4_h26x_writer_t*)calloc(1, sizeof(mp4_h26x_writer_t));
        if (!enc->mp4_writer)
        {
            log_error("failed to allocate mp4 writer");
            return false;
        }
    }
    memset(enc->mp4_writer, 0, sizeof(*enc->mp4_writer));
    mp4_h26x_write_init(
        enc->mp4_writer, enc->mp4_mux, (int)enc->cfg.width, (int)enc->cfg.height,
        enc->cfg.codec == DVZ_VIDEO_CODEC_HEVC);
    enc->mp4_writer_ready = true;
    enc->mp4_frame_duration_num = 90000;
    enc->mp4_frame_duration_den = enc->cfg.fps > 0 ? enc->cfg.fps : 1;
    enc->mp4_frame_duration_accum = 0;
    return true;
}

static int dvz_video_encoder_mux_post(DvzVideoEncoder* enc)
{
    ANN(enc);
    if (enc->mux != DVZ_VIDEO_MUX_MP4_POST || enc->mux_sample_count == 0 || !enc->fp)
    {
        return 0;
    }
    const char* mp4_path = enc->cfg.mp4_path ? enc->cfg.mp4_path : "out.mp4";
    FILE* mp4_fp = fopen(mp4_path, "wb");
    if (!mp4_fp)
    {
        log_error("failed to open mp4 output '%s': %s", mp4_path, strerror(errno));
        return -1;
    }

    MP4E_mux_t* mux = MP4E_open(1, 0, mp4_fp, dvz_mp4_write_cb);
    if (!mux)
    {
        log_error("failed to initialize minimp4 muxer");
        fclose(mp4_fp);
        return -1;
    }

    mp4_h26x_writer_t writer = {0};
    mp4_h26x_write_init(
        &writer, mux, (int)enc->cfg.width, (int)enc->cfg.height,
        enc->cfg.codec == DVZ_VIDEO_CODEC_HEVC);

    uint8_t* buffer = NULL;
    size_t buffer_size = 0;
    for (size_t i = 0; i < enc->mux_sample_count; ++i)
    {
        const DvzMuxSample sample = enc->mux_samples[i];
        if (sample.size == 0)
        {
            continue;
        }
        if (sample.size > buffer_size)
        {
            uint8_t* new_buf = (uint8_t*)realloc(buffer, sample.size);
            if (!new_buf)
            {
                log_error("failed to grow mux scratch buffer");
                free(buffer);
                mp4_h26x_write_close(&writer);
                MP4E_close(mux);
                fclose(mp4_fp);
                return -1;
            }
            buffer = new_buf;
            buffer_size = sample.size;
        }
        if (fseeko(enc->fp, (off_t)sample.offset, SEEK_SET) != 0)
        {
            log_error("failed to seek raw stream for mp4 mux");
            free(buffer);
            mp4_h26x_write_close(&writer);
            MP4E_close(mux);
            fclose(mp4_fp);
            return -1;
        }
        size_t read = fread(buffer, 1, sample.size, enc->fp);
        if (read != sample.size)
        {
            log_error("failed to read raw stream chunk for mp4 mux");
            free(buffer);
            mp4_h26x_write_close(&writer);
            MP4E_close(mux);
            fclose(mp4_fp);
            return -1;
        }
        int err = mp4_h26x_write_nal(&writer, buffer, (int)sample.size, sample.duration);
        if (err != MP4E_STATUS_OK)
        {
            log_error("minimp4 post-processing failed with status %d", err);
            free(buffer);
            mp4_h26x_write_close(&writer);
            MP4E_close(mux);
            fclose(mp4_fp);
            return -1;
        }
    }
    free(buffer);
    mp4_h26x_write_close(&writer);
    MP4E_close(mux);
    fclose(mp4_fp);
    return 0;
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
        .color_format = VK_FORMAT_R8G8B8A8_UNORM,
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
    DvzVideoEncoder* enc = (DvzVideoEncoder*)calloc(1, sizeof(DvzVideoEncoder));
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
        free(enc);
        return NULL;
    }

    return enc;
}

int dvz_video_encoder_start(
    DvzVideoEncoder* enc,
    VkImage image,
    VkDeviceMemory memory,
    VkDeviceSize memory_size,
    int memory_fd,
    int wait_semaphore_fd,
    FILE* bitstream_out)
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
        bitstream_out = fopen(raw_path, "wb");
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
    dvz_video_encoder_release(enc);
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
    dvz_video_encoder_release(enc);
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
        free(enc->mp4_writer);
        enc->mp4_writer = NULL;
    }
    free(enc);
}



/*************************************************************************************************/
/*  Backend helpers                                                                              */
/*************************************************************************************************/

void dvz_video_encoder_on_sample(
    DvzVideoEncoder* enc,
    const uint8_t* data,
    uint32_t size,
    uint64_t file_offset,
    uint32_t duration,
    bool keyframe)
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

static void dvz_video_encoder_release(DvzVideoEncoder* enc)
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
        free(enc->mp4_path_owned);
        enc->mp4_path_owned = NULL;
    }
    free(enc->mux_samples);
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
