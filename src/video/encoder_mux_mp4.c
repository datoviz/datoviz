/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  MP4 mux helpers                                                                              */
/*************************************************************************************************/

#include "encoder_internal.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>

MUTE_ON
#define MINIMP4_IMPLEMENTATION
#include "minimp4.h"
MUTE_OFF

#include "_alloc.h"
#include "_log.h"
#include "datoviz/common/macros.h"



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

static int mp4_write_cb(int64_t offset, const void* buffer, size_t size, void* token)
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



/*************************************************************************************************/
/*  Mux functions                                                                                */
/*************************************************************************************************/

bool dvz_video_encoder_open_mp4_stream(DvzVideoEncoder* enc)
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
    enc->mp4_mux = MP4E_open(1, 0, enc->mp4_fp, mp4_write_cb);
    if (!enc->mp4_mux)
    {
        log_error("failed to initialize minimp4 muxer");
        return false;
    }
    if (!enc->mp4_writer)
    {
        enc->mp4_writer = (mp4_h26x_writer_t*)dvz_calloc(1, sizeof(mp4_h26x_writer_t));
        if (!enc->mp4_writer)
        {
            log_error("failed to allocate mp4 writer");
            return false;
        }
    }
    dvz_memset(enc->mp4_writer, sizeof(*enc->mp4_writer), 0, sizeof(*enc->mp4_writer));
    mp4_h26x_write_init(
        enc->mp4_writer, enc->mp4_mux, (int)enc->cfg.width, (int)enc->cfg.height,
        enc->cfg.codec == DVZ_VIDEO_CODEC_HEVC);
    enc->mp4_writer_ready = true;
    enc->mp4_frame_duration_num = 90000;
    enc->mp4_frame_duration_den = enc->cfg.fps > 0 ? enc->cfg.fps : 1;
    enc->mp4_frame_duration_accum = 0;
    return true;
}



void dvz_video_encoder_close_mp4(DvzVideoEncoder* enc)
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



void dvz_video_encoder_mux_sample(
    DvzVideoEncoder* enc, const uint8_t* data, uint32_t size, uint32_t duration)
{
    if (!enc || enc->mux != DVZ_VIDEO_MUX_MP4_STREAMING || !enc->mp4_writer_ready)
    {
        return;
    }
    if (!data || size == 0 || !enc->mp4_writer)
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



void dvz_video_encoder_record_sample(
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
            (DvzMuxSample*)dvz_realloc(enc->mux_samples, new_cap * sizeof(DvzMuxSample));
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



int dvz_video_encoder_mux_post(DvzVideoEncoder* enc)
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

    MP4E_mux_t* mux = MP4E_open(1, 0, mp4_fp, mp4_write_cb);
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
            uint8_t* new_buf = (uint8_t*)dvz_realloc(buffer, sample.size);
            if (!new_buf)
            {
                log_error("failed to grow mux scratch buffer");
                dvz_free(buffer);
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
            log_error(
                "failed to seek raw stream for mp4 mux (sample %zu, offset=%llu)", i,
                (unsigned long long)sample.offset);
            dvz_free(buffer);
            mp4_h26x_write_close(&writer);
            MP4E_close(mux);
            fclose(mp4_fp);
            return -1;
        }
        size_t read = fread(buffer, 1, sample.size, enc->fp);
        if (read != sample.size)
        {
            log_error(
                "failed to read raw stream chunk for mp4 mux (sample %zu, size=%u)", i,
                sample.size);
            dvz_free(buffer);
            mp4_h26x_write_close(&writer);
            MP4E_close(mux);
            fclose(mp4_fp);
            return -1;
        }
        int err = mp4_h26x_write_nal(&writer, buffer, (int)sample.size, sample.duration);
        if (err != MP4E_STATUS_OK)
        {
            log_error(
                "minimp4 post-processing failed with status %d (sample %zu, duration=%u)", err, i,
                sample.duration);
            dvz_free(buffer);
            mp4_h26x_write_close(&writer);
            MP4E_close(mux);
            fclose(mp4_fp);
            return -1;
        }
    }
    dvz_free(buffer);
    mp4_h26x_write_close(&writer);
    MP4E_close(mux);
    fclose(mp4_fp);
    return 0;
}
