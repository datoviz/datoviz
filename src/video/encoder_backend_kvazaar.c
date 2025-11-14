/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Kvazaar backend placeholder                                                                  */
/*************************************************************************************************/

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <volk.h>

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#else
#include <unistd.h>
#endif

#include "_alloc.h"
#include "_log.h"
#include "datoviz/common/macros.h"
#include "datoviz/thread/thread.h"
#include "datoviz/vk/device.h"
#include "encoder_backend.h"
#include "kvazaar.h"



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

// Kvazaar video backend.
typedef struct
{
    const kvz_api* api;
    kvz_config* cfg;
    kvz_encoder* encoder;

    VkDevice device;
    VkSemaphore wait_semaphore;
    bool wait_semaphore_ready;

    void* mapped_ptr;
    VkMappedMemoryRange mapped_range;
    VkSubresourceLayout layout;
    bool mapped;

    uint32_t width;
    uint32_t height;
    VkFormat format;
    uint32_t convert_threads;
} DvzVideoBackendKvazaar;



// RGBA YUV conversion job.
typedef struct
{
    DvzVideoBackendKvazaar* state;
    kvz_picture* picture;
    const uint8_t* base;
    size_t src_stride;
    uint32_t row_start;
    uint32_t row_end;
    int result;
} DvzKvazaarConvertJob;



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

static DvzVideoBackendKvazaar* kvazaar_state(DvzVideoEncoder* enc)
{
    ANN(enc);
    return (DvzVideoBackendKvazaar*)enc->backend_data;
}



static uint32_t kvazaar_min_u32(uint32_t a, uint32_t b) { return (a < b) ? a : b; }



static uint32_t kvazaar_max_u32(uint32_t a, uint32_t b) { return (a > b) ? a : b; }



static uint32_t kvazaar_cpu_core_count(void)
{
#if defined(_WIN32)
    SYSTEM_INFO info;
    GetSystemInfo(&info);
    return (info.dwNumberOfProcessors > 0) ? info.dwNumberOfProcessors : 1u;
#elif defined(_SC_NPROCESSORS_ONLN)
    long count = sysconf(_SC_NPROCESSORS_ONLN);
    if (count < 1)
    {
#ifdef _SC_NPROCESSORS_CONF
        count = sysconf(_SC_NPROCESSORS_CONF);
#endif
    }
    if (count < 1)
    {
        count = 1;
    }
    return (uint32_t)count;
#else
    return 1;
#endif
}



static inline uint8_t kvazaar_clamp(int value)
{
    if (value < 0)
    {
        return 0;
    }
    if (value > 255)
    {
        return 255;
    }
    return (uint8_t)value;
}



static bool kvazaar_is_keyframe(const kvz_frame_info* info, bool force_idr)
{
    if (!info)
    {
        return force_idr;
    }
    switch (info->nal_unit_type)
    {
    case KVZ_NAL_IDR_W_RADL:
    case KVZ_NAL_IDR_N_LP:
    case KVZ_NAL_CRA_NUT:
        return true;
    default:
        break;
    }
    return (info->slice_type == KVZ_SLICE_I) || force_idr;
}



static int kvazaar_map_image(DvzVideoEncoder* enc, DvzVideoBackendKvazaar* state)
{
    ANN(enc);
    ANN(state);
    if (!state->device || enc->memory == VK_NULL_HANDLE)
    {
        log_error("kvazaar backend requires a valid Vulkan device and memory handle");
        return -1;
    }
    VkResult res =
        vkMapMemory(state->device, enc->memory, 0, enc->memory_size, 0, &state->mapped_ptr);
    if (res != VK_SUCCESS || !state->mapped_ptr)
    {
        log_error(
            "kvazaar backend requires host-visible memory (vkMapMemory failed with %d)", res);
        return -1;
    }
    state->mapped = true;
    state->mapped_range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    state->mapped_range.pNext = NULL;
    state->mapped_range.memory = enc->memory;
    state->mapped_range.offset = 0;
    state->mapped_range.size = enc->memory_size;

    VkImageSubresource sub = {
        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .mipLevel = 0, .arrayLayer = 0};
    vkGetImageSubresourceLayout(state->device, enc->image, &sub, &state->layout);
    if (state->layout.rowPitch == 0)
    {
        log_error("vkGetImageSubresourceLayout returned rowPitch=0 for kvazaar backend");
        return -1;
    }
    return 0;
}



static void kvazaar_unmap_image(DvzVideoEncoder* enc, DvzVideoBackendKvazaar* state)
{
    ANN(state);
    if (state->mapped && state->device && enc && enc->memory != VK_NULL_HANDLE)
    {
        vkUnmapMemory(state->device, enc->memory);
    }
    state->mapped = false;
    state->mapped_ptr = NULL;
}



static int kvazaar_import_semaphore(DvzVideoEncoder* enc, DvzVideoBackendKvazaar* state)
{
    ANN(state);
    if (!enc || enc->wait_semaphore_fd < 0)
    {
        return 0;
    }
    if (!vkImportSemaphoreFdKHR || !vkWaitSemaphores)
    {
        log_warn("timeline semaphore import unavailable; kvazaar backend will fall back to "
                 "vkDeviceWaitIdle");
        return 0;
    }
    VkSemaphoreTypeCreateInfo timeline_info = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO,
        .pNext = NULL,
        .semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE,
        .initialValue = 0,
    };
    VkSemaphoreCreateInfo sci = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO, .pNext = &timeline_info};
    VkResult res = vkCreateSemaphore(state->device, &sci, NULL, &state->wait_semaphore);
    if (res != VK_SUCCESS)
    {
        log_error("failed to create timeline semaphore for kvazaar backend (%d)", res);
        return -1;
    }

    VkImportSemaphoreFdInfoKHR import_info = {
        .sType = VK_STRUCTURE_TYPE_IMPORT_SEMAPHORE_FD_INFO_KHR,
        .pNext = NULL,
        .semaphore = state->wait_semaphore,
        .flags = 0,
        .handleType = VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_FD_BIT,
        .fd = enc->wait_semaphore_fd,
    };
    res = vkImportSemaphoreFdKHR(state->device, &import_info);
    if (res != VK_SUCCESS)
    {
        log_error(
            "vkImportSemaphoreFdKHR failed (%d); kvazaar backend cannot wait on GPU timeline",
            res);
        vkDestroySemaphore(state->device, state->wait_semaphore, NULL);
        state->wait_semaphore = VK_NULL_HANDLE;
        return -1;
    }
    state->wait_semaphore_ready = true;
    return 0;
}



static int kvazaar_wait_for_signal(DvzVideoBackendKvazaar* state, uint64_t wait_value)
{
    ANN(state);
    if (wait_value == 0)
    {
        return 0;
    }
    if (state->wait_semaphore_ready && state->wait_semaphore != VK_NULL_HANDLE)
    {
        VkSemaphoreWaitInfo wait_info = {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO,
            .pNext = NULL,
            .flags = 0,
            .semaphoreCount = 1,
            .pSemaphores = &state->wait_semaphore,
            .pValues = &wait_value,
        };
        VkResult res = vkWaitSemaphores(state->device, &wait_info, UINT64_MAX);
        if (res != VK_SUCCESS)
        {
            log_error("vkWaitSemaphores failed for kvazaar backend (%d)", res);
            return -1;
        }
        return 0;
    }
    // Fallback: block until the queue is idle.
    VkResult res = vkDeviceWaitIdle(state->device);
    if (res != VK_SUCCESS)
    {
        log_error("vkDeviceWaitIdle failed for kvazaar backend (%d)", res);
        return -1;
    }
    return 0;
}



static kvz_picture* kvazaar_alloc_picture(DvzVideoBackendKvazaar* state)
{
    ANN(state);
    if (!state->api || !state->api->picture_alloc)
    {
        return NULL;
    }
    if (state->api->picture_alloc_csp)
    {
        return state->api->picture_alloc_csp(
            KVZ_CSP_420, (int32_t)state->width, (int32_t)state->height);
    }
    return state->api->picture_alloc((int32_t)state->width, (int32_t)state->height);
}



static bool kvazaar_convert_rgba_to_yuv_rows(
    DvzVideoBackendKvazaar* state, kvz_picture* picture, const uint8_t* base, size_t src_stride,
    uint32_t row_start, uint32_t row_end)
{
    ANN(state);
    ANN(picture);
    if (!base)
    {
        return false;
    }
    const uint32_t width = state->width;
    const uint32_t height = state->height;
    if (width == 0 || height == 0)
    {
        return false;
    }
    if (picture->stride < (int32_t)width)
    {
        log_error("kvazaar picture stride (%d) smaller than width (%u)", picture->stride, width);
        return false;
    }
    if (row_end > height)
    {
        row_end = height;
    }
    if (row_start >= row_end)
    {
        return true;
    }
    if ((row_start & 1u) != 0)
    {
        row_start -= 1;
    }
    if ((row_end & 1u) != 0)
    {
        row_end -= 1;
    }
    if (row_end <= row_start)
    {
        return true;
    }
    kvz_pixel* y_plane = picture->y;
    kvz_pixel* u_plane = picture->u;
    kvz_pixel* v_plane = picture->v;
    const int y_stride = picture->stride;
    const int uv_stride = picture->stride / 2;

    for (uint32_t row = row_start; row < row_end; row += 2)
    {
        const uint8_t* row0 = base + (size_t)row * src_stride;
        const uint8_t* row1 = base + (size_t)(row + 1) * src_stride;
        kvz_pixel* y0 = y_plane + (ptrdiff_t)row * y_stride;
        kvz_pixel* y1 = y_plane + (ptrdiff_t)(row + 1) * y_stride;
        kvz_pixel* u_row = u_plane + (ptrdiff_t)(row / 2) * uv_stride;
        kvz_pixel* v_row = v_plane + (ptrdiff_t)(row / 2) * uv_stride;

        for (uint32_t col = 0; col < width; col += 2)
        {
            const uint8_t* p0 = row0 + col * 4;
            const uint8_t* p1 = row0 + (col + 1) * 4;
            const uint8_t* p2 = row1 + col * 4;
            const uint8_t* p3 = row1 + (col + 1) * 4;

            const uint8_t r[4] = {p0[0], p1[0], p2[0], p3[0]};
            const uint8_t g[4] = {p0[1], p1[1], p2[1], p3[1]};
            const uint8_t b[4] = {p0[2], p1[2], p2[2], p3[2]};

            const uint8_t* px = p0;
            int y_val = ((66 * px[0] + 129 * px[1] + 25 * px[2] + 128) >> 8) + 16;
            y0[col] = kvazaar_clamp(y_val);
            px = p1;
            y_val = ((66 * px[0] + 129 * px[1] + 25 * px[2] + 128) >> 8) + 16;
            y0[col + 1] = kvazaar_clamp(y_val);

            px = p2;
            y_val = ((66 * px[0] + 129 * px[1] + 25 * px[2] + 128) >> 8) + 16;
            y1[col] = kvazaar_clamp(y_val);
            px = p3;
            y_val = ((66 * px[0] + 129 * px[1] + 25 * px[2] + 128) >> 8) + 16;
            y1[col + 1] = kvazaar_clamp(y_val);

            uint8_t u_values[4];
            uint8_t v_values[4];
            for (uint32_t i = 0; i < 4; ++i)
            {
                int r_val = r[i];
                int g_val = g[i];
                int b_val = b[i];
                int u_val = ((-38 * r_val - 74 * g_val + 112 * b_val + 128) >> 8) + 128;
                int v_val = ((112 * r_val - 94 * g_val - 18 * b_val + 128) >> 8) + 128;
                u_values[i] = kvazaar_clamp(u_val);
                v_values[i] = kvazaar_clamp(v_val);
            }
            const uint32_t chroma_col = col / 2;
            u_row[chroma_col] =
                kvazaar_clamp((u_values[0] + u_values[1] + u_values[2] + u_values[3]) / 4);
            v_row[chroma_col] =
                kvazaar_clamp((v_values[0] + v_values[1] + v_values[2] + v_values[3]) / 4);
        }
    }
    return true;
}



static void* kvazaar_convert_thread(void* user_data)
{
    DvzKvazaarConvertJob* job = (DvzKvazaarConvertJob*)user_data;
    ANN(job);
    job->result =
        kvazaar_convert_rgba_to_yuv_rows(
            job->state, job->picture, job->base, job->src_stride, job->row_start, job->row_end)
            ? 0
            : -1;
    return NULL;
}



static bool kvazaar_convert_rgba_to_yuv(
    DvzVideoBackendKvazaar* state, kvz_picture* picture, const uint8_t* base, size_t src_stride)
{
    ANN(state);
    ANN(picture);
    if (!base)
    {
        return false;
    }
    const uint32_t height = state->height;
    if (state->convert_threads <= 1 || height <= 2)
    {
        return kvazaar_convert_rgba_to_yuv_rows(state, picture, base, src_stride, 0, height);
    }

    uint32_t row_pairs = height >> 1;
    uint32_t thread_count = kvazaar_min_u32(state->convert_threads, row_pairs);
    if (thread_count == 0)
    {
        return kvazaar_convert_rgba_to_yuv_rows(state, picture, base, src_stride, 0, height);
    }
    uint32_t rows_per_thread = ((row_pairs + thread_count - 1) / thread_count);
    rows_per_thread = kvazaar_max_u32(rows_per_thread, 1u) << 1u;

    DvzKvazaarConvertJob* jobs =
        (DvzKvazaarConvertJob*)dvz_calloc(thread_count, sizeof(DvzKvazaarConvertJob));
    DvzThread** threads = (DvzThread**)dvz_calloc(thread_count, sizeof(DvzThread*));
    ANN(jobs);
    ANN(threads);

    uint32_t launched = 0;
    for (uint32_t row = 0; row < height && launched < thread_count; row += rows_per_thread)
    {
        uint32_t row_end = row + rows_per_thread;
        if (row_end > height)
        {
            row_end = height;
        }
        DvzKvazaarConvertJob* job = &jobs[launched];
        job->state = state;
        job->picture = picture;
        job->base = base;
        job->src_stride = src_stride;
        job->row_start = row;
        job->row_end = row_end;
        job->result = -1;
        threads[launched] = dvz_thread(kvazaar_convert_thread, job);
        launched++;
    }

    bool ok = true;
    for (uint32_t i = 0; i < launched; ++i)
    {
        if (threads[i])
        {
            dvz_thread_join(threads[i]);
        }
        if (jobs[i].result != 0)
        {
            ok = false;
        }
    }

    dvz_free(threads);
    dvz_free(jobs);
    return ok;
}



static int kvazaar_emit_sample(
    DvzVideoEncoder* enc, kvz_data_chunk* chunks, uint32_t total_size, uint32_t duration,
    bool keyframe)
{
    ANN(enc);
    if (!chunks || total_size == 0)
    {
        return 0;
    }
    uint8_t* buffer = (uint8_t*)malloc(total_size);
    if (!buffer)
    {
        log_error("failed to allocate temporary buffer for kvazaar sample (%u bytes)", total_size);
        return -1;
    }
    uint32_t copied = 0;
    for (kvz_data_chunk* chunk = chunks; chunk != NULL; chunk = chunk->next)
    {
        if (copied + chunk->len > total_size)
        {
            free(buffer);
            log_error("kvazaar chunk list larger than reported total size");
            return -1;
        }
        dvz_memcpy(buffer + copied, chunk->len, chunk->data, chunk->len);
        copied += chunk->len;
    }

    uint64_t file_offset = UINT64_MAX;
    if (enc->fp && enc->mux == DVZ_VIDEO_MUX_MP4_POST)
    {
        long pos = ftello(enc->fp);
        if (pos >= 0)
        {
            file_offset = (uint64_t)pos;
        }
    }
    if (enc->fp)
    {
        fwrite(buffer, 1, total_size, enc->fp);
    }
    dvz_video_encoder_on_sample(enc, buffer, total_size, file_offset, duration, keyframe);
    free(buffer);
    return 0;
}



static void kvazaar_drain(DvzVideoEncoder* enc, DvzVideoBackendKvazaar* state)
{
    ANN(enc);
    ANN(state);
    if (!state->encoder || !state->api)
    {
        return;
    }
    while (true)
    {
        kvz_data_chunk* chunks = NULL;
        kvz_picture* pic_rec = NULL;
        kvz_picture* pic_src = NULL;
        kvz_frame_info info = {0};
        uint32_t len_out = 0;
        if (!state->api->encoder_encode(
                state->encoder, NULL, &chunks, &len_out, &pic_rec, &pic_src, &info))
        {
            log_error("kvazaar flush failed");
            state->api->picture_free(pic_rec);
            state->api->picture_free(pic_src);
            state->api->chunk_free(chunks);
            break;
        }
        bool has_chunks = (chunks != NULL && len_out > 0);
        if (has_chunks)
        {
            uint32_t duration = dvz_video_encoder_next_duration(enc);
            kvazaar_emit_sample(enc, chunks, len_out, duration, kvazaar_is_keyframe(&info, false));
        }
        state->api->picture_free(pic_rec);
        state->api->picture_free(pic_src);
        state->api->chunk_free(chunks);
        if (!has_chunks)
        {
            break;
        }
    }
}



/*************************************************************************************************/
/*  Public backend                                                                               */
/*************************************************************************************************/

static bool kvazaar_probe(const DvzVideoEncoderConfig* cfg)
{
    if (!cfg)
    {
        return true;
    }
    if (cfg->codec != DVZ_VIDEO_CODEC_HEVC)
    {
        log_warn("kvazaar backend only supports HEVC/H.265, requested codec=%d", cfg->codec);
        return false;
    }
    if ((cfg->width % 2) != 0 || (cfg->height % 2) != 0)
    {
        log_warn(
            "kvazaar backend requires even width/height (got %ux%u)", cfg->width, cfg->height);
        return false;
    }
    return true;
}



static int kvazaar_init(DvzVideoEncoder* enc)
{
    ANN(enc);
    if (enc->backend_data)
    {
        return 0;
    }
    const kvz_api* api = kvz_api_get(8);
    if (!api)
    {
        log_error("kvazaar API unavailable");
        return -1;
    }
    kvz_config* cfg = api->config_alloc();
    if (!cfg)
    {
        log_error("failed to allocate kvazaar config");
        return -1;
    }
    if (!api->config_init(cfg))
    {
        log_error("kvazaar config initialization failed");
        api->config_destroy(cfg);
        return -1;
    }

    DvzVideoBackendKvazaar* state =
        (DvzVideoBackendKvazaar*)calloc(1, sizeof(DvzVideoBackendKvazaar));
    ANN(state);
    state->api = api;
    state->cfg = cfg;
    state->wait_semaphore = VK_NULL_HANDLE;
    enc->backend_data = state;
    return 0;
}



static int kvazaar_start(DvzVideoEncoder* enc)
{
    ANN(enc);
    DvzVideoBackendKvazaar* state = kvazaar_state(enc);
    if (!state)
    {
        if (kvazaar_init(enc) != 0)
        {
            return -1;
        }
        state = kvazaar_state(enc);
    }
    ANN(state);
    if (!enc->device || enc->device->vk_device == VK_NULL_HANDLE)
    {
        log_error("kvazaar backend requires a valid DvzDevice (vk_device cannot be NULL)");
        return -1;
    }
    if (enc->image == VK_NULL_HANDLE || enc->memory == VK_NULL_HANDLE)
    {
        log_error("kvazaar backend requires a bound VkImage and VkDeviceMemory");
        return -1;
    }
    state->device = enc->device->vk_device;
    state->width = enc->cfg.width;
    state->height = enc->cfg.height;
    state->format = enc->cfg.color_format;
    uint32_t cpu_cores = kvazaar_cpu_core_count();
    uint32_t conversion_threads = (cpu_cores > 1) ? (cpu_cores / 2) : 1;
    conversion_threads = kvazaar_min_u32(conversion_threads, state->height / 2);
    if (conversion_threads == 0)
    {
        conversion_threads = 1;
    }
    state->convert_threads = conversion_threads;
    if (state->format != VK_FORMAT_R8G8B8A8_UNORM)
    {
        log_error("kvazaar backend currently only supports VK_FORMAT_R8G8B8A8_UNORM images");
        return -1;
    }
    if ((state->width % 2) != 0 || (state->height % 2) != 0)
    {
        log_error("kvazaar backend requires even width and height");
        return -1;
    }

    kvz_config* cfg = state->cfg;
    ANN(cfg);
    cfg->width = (int32_t)state->width;
    cfg->height = (int32_t)state->height;
    cfg->framerate_num = (int32_t)(enc->cfg.fps > 0 ? enc->cfg.fps : 60);
    cfg->framerate_denom = 1;
    cfg->framerate = (double)cfg->framerate_num / (double)cfg->framerate_denom;
    cfg->input_bitdepth = 8;
    cfg->input_format = KVZ_FORMAT_P420;
    cfg->intra_period = cfg->framerate_num * 2;
    uint32_t gop = cfg->gop_len > 0 ? (uint32_t)cfg->gop_len : 1;
    int32_t aligned = (cfg->intra_period / (int32_t)gop) * (int32_t)gop;
    if (aligned <= 0)
    {
        aligned = (int32_t)gop;
    }
    cfg->intra_period = aligned;
    cfg->gop_len = (int8_t)gop;
    cfg->vps_period = cfg->intra_period;
    cfg->target_bitrate = 0;
    cfg->qp = 22;
    cfg->rdo = 2;
    if (cfg->threads <= 0)
    {
        const uint32_t cpu_threads = kvazaar_min_u32(cpu_cores, 64);
        cfg->threads = (int32_t)(cpu_threads > 0 ? cpu_threads : 1);
    }
    log_debug(
        "kvazaar backend using %d encoder threads and %u conversion workers", cfg->threads,
        state->convert_threads);
    cfg->aud_enable = 0;
    cfg->add_encoder_info = 0;

    if (kvazaar_map_image(enc, state) != 0)
    {
        return -1;
    }
    if (kvazaar_import_semaphore(enc, state) != 0)
    {
        kvazaar_unmap_image(enc, state);
        return -1;
    }

    state->encoder = state->api->encoder_open(cfg);
    if (!state->encoder)
    {
        log_error("failed to open kvazaar encoder");
        kvazaar_unmap_image(enc, state);
        return -1;
    }

    kvz_data_chunk* headers = NULL;
    uint32_t header_size = 0;
    if (state->api->encoder_headers(state->encoder, &headers, &header_size))
    {
        if (headers && header_size > 0)
        {
            kvazaar_emit_sample(enc, headers, header_size, 0, true);
        }
    }
    state->api->chunk_free(headers);
    return 0;
}



static int kvazaar_submit(DvzVideoEncoder* enc, uint64_t timeline_value)
{
    ANN(enc);
    DvzVideoBackendKvazaar* state = kvazaar_state(enc);
    if (!state || !state->encoder)
    {
        return -1;
    }
    if (kvazaar_wait_for_signal(state, timeline_value) != 0)
    {
        return -1;
    }
    if (state->mapped && state->mapped_range.size > 0)
    {
        vkInvalidateMappedMemoryRanges(state->device, 1, &state->mapped_range);
    }
    kvz_picture* picture = kvazaar_alloc_picture(state);
    if (!picture)
    {
        log_error("failed to allocate kvazaar picture");
        return -1;
    }
    const uint8_t* base =
        state->mapped ? ((const uint8_t*)state->mapped_ptr + (size_t)state->layout.offset) : NULL;
    if (!kvazaar_convert_rgba_to_yuv(state, picture, base, (size_t)state->layout.rowPitch))
    {
        state->api->picture_free(picture);
        return -1;
    }
    picture->pts = (int64_t)enc->frame_idx;

    kvz_data_chunk* chunks = NULL;
    kvz_picture* pic_rec = NULL;
    kvz_picture* pic_src = NULL;
    kvz_frame_info info = {0};
    uint32_t len_out = 0;
    if (!state->api->encoder_encode(
            state->encoder, picture, &chunks, &len_out, &pic_rec, &pic_src, &info))
    {
        log_error("kvazaar encoder_encode failed");
        state->api->picture_free(picture);
        state->api->picture_free(pic_rec);
        state->api->picture_free(pic_src);
        state->api->chunk_free(chunks);
        return -1;
    }
    uint32_t duration = dvz_video_encoder_next_duration(enc);
    if (chunks && len_out > 0)
    {
        kvazaar_emit_sample(
            enc, chunks, len_out, duration, kvazaar_is_keyframe(&info, enc->frame_idx == 0));
    }
    state->api->picture_free(picture);
    state->api->picture_free(pic_rec);
    state->api->picture_free(pic_src);
    state->api->chunk_free(chunks);
    return 0;
}



static int kvazaar_stop(DvzVideoEncoder* enc)
{
    if (!enc)
    {
        return 0;
    }
    DvzVideoBackendKvazaar* state = kvazaar_state(enc);
    if (!state)
    {
        return 0;
    }
    kvazaar_drain(enc, state);
    return 0;
}



static void kvazaar_destroy(DvzVideoEncoder* enc)
{
    if (!enc || !enc->backend_data)
    {
        return;
    }
    DvzVideoBackendKvazaar* state = kvazaar_state(enc);
    if (state->api)
    {
        if (state->encoder)
        {
            state->api->encoder_close(state->encoder);
            state->encoder = NULL;
        }
        if (state->cfg)
        {
            state->api->config_destroy(state->cfg);
            state->cfg = NULL;
        }
    }
    if (state->wait_semaphore != VK_NULL_HANDLE)
    {
        vkDestroySemaphore(state->device, state->wait_semaphore, NULL);
        state->wait_semaphore = VK_NULL_HANDLE;
        state->wait_semaphore_ready = false;
    }
    kvazaar_unmap_image(enc, state);
    free(state);
    enc->backend_data = NULL;
}



const DvzVideoBackend DVZ_VIDEO_BACKEND_KVAZAAR = {
    .name = "kvazaar",
    .probe = kvazaar_probe,
    .init = kvazaar_init,
    .start = kvazaar_start,
    .submit = kvazaar_submit,
    .stop = kvazaar_stop,
    .destroy = kvazaar_destroy,
};
