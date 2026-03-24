/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Testing buffers                                                                              */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdint.h>

#include "test_vk.h"
#include "_alloc.h"
#include "_assertions.h"
#include "_compat.h"
#include "datoviz/vk/gpu_ctx.h"
#include "datoviz/vklite/buffers.h"
#include "test_vklite.h"
#include "datoviz/math/types.h"
#include "datoviz/vk/instance.h"
#include "testing.h"
#include "vulkan_core.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define MAP_OFFSET 64
#define MAP_SIZE   1024



/*************************************************************************************************/
/*  Tests                                                                                        */
/*************************************************************************************************/

int test_vklite_buffers_1(TstSuite* suite, TstItem* tstitem)
{
    ANN(suite);
    ANN(tstitem);

    // Bootstrap.
    DvzGpuCtxConfig cfg = dvz_gpu_ctx_config();
    DvzGpuCtx* ctx = dvz_gpu_ctx(&cfg);
    ANN(ctx);

    DvzBuffer* buffer = dvz_buffer_create_wrapper();
    ANN(buffer);
    DvzSize size = 65536;

    dvz_buffer(dvz_gpu_ctx_device(ctx), dvz_gpu_ctx_alloc(ctx), buffer);
    dvz_buffer_size(buffer, size);
    dvz_buffer_flags(buffer, DVZ_ALLOC_HOST_ACCESS_SEQUENTIAL_WRITE);
    dvz_buffer_usage(buffer, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
    AT(dvz_buffer_size_value(buffer) == size);
    dvz_buffer_create(buffer);

    // Map the buffer.
    dvz_buffer_map(buffer);

    // Create some data.
    uint8_t data[MAP_SIZE] = {0};
    DvzSize msize = MAP_SIZE;
    for (uint32_t i = 0; i < MAP_SIZE; i++)
    {
        data[i] = i;
    }
    DvzSize offset = MAP_OFFSET;

    // Upload the data and check.
    dvz_buffer_upload(buffer, offset, msize, data);
    AT(data[10] == 10);

    // Reset the data.
    dvz_buffer_unmap(buffer);
    dvz_memset(data, msize, 0, msize);
    AT(data[10] == 0);

    // Download the data and check again.
    dvz_buffer_download(buffer, offset, msize, data);
    AT(data[10] == 10);


    // RESIZING.

    // No-op as buffer is smaller.
    dvz_buffer_resize(buffer, size / 2);

    // Download the data and check again.
    dvz_buffer_download(buffer, offset, msize, data);
    AT(data[10] == 10);

    // Buffer recreated if size is larger.
    dvz_buffer_resize(buffer, 2 * size);

    // Download the data and check again.
    dvz_buffer_download(buffer, offset, msize, data);
    AT(dvz_buffer_allocated_size(buffer) == 2 * size);
    AT(dvz_buffer_handle(buffer) != VK_NULL_HANDLE);

    // Cleanup.
    dvz_buffer_destroy(buffer);
    dvz_buffer_destroy(buffer);
    AT(dvz_buffer_handle(buffer) == VK_NULL_HANDLE);

    AT(dvz_buffer_create(buffer) == 0);
    AT(dvz_buffer_handle(buffer) != VK_NULL_HANDLE);
    AT(dvz_buffer_allocated_size(buffer) == 2 * size);
    dvz_buffer_destroy(buffer);
    AT(dvz_buffer_handle(buffer) == VK_NULL_HANDLE);

    dvz_buffer_free(buffer);
    uint32_t err_count = dvz_gpu_ctx_error_count(ctx);
    dvz_gpu_ctx_destroy(ctx);

    return err_count > 0;
}



int test_vklite_buffer_views(TstSuite* suite, TstItem* tstitem)
{
    // Bootstrap.
    DvzGpuCtxConfig cfg = dvz_gpu_ctx_config();
    DvzGpuCtx* ctx = dvz_gpu_ctx(&cfg);
    ANN(ctx);

    DvzBuffer* buffer = dvz_buffer_create_wrapper();
    ANN(buffer);
    DvzSize size = 65536;

    dvz_buffer(dvz_gpu_ctx_device(ctx), dvz_gpu_ctx_alloc(ctx), buffer);
    dvz_buffer_size(buffer, size);
    dvz_buffer_flags(buffer, DVZ_ALLOC_HOST_ACCESS_SEQUENTIAL_WRITE);
    dvz_buffer_usage(buffer, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
    AT(dvz_buffer_size_value(buffer) == size);
    dvz_buffer_create(buffer);

    DvzBufferViews* views = dvz_buffer_views_create();
    ANN(views);
    DvzSize offset = 33;
    DvzSize vsize = 7;
    DvzSize alignment = 16;
    dvz_buffer_views(buffer, 3, offset, vsize, alignment, views);
    AT(dvz_buffer_views_count(views) == 3);
    AT(dvz_buffer_views_size(views) == vsize);
    AT(dvz_buffer_views_aligned_size(views) == 16);
    AT(dvz_buffer_views_offset(views, 0) == 48);
    AT(dvz_buffer_views_offset(views, 1) == 64);
    AT(dvz_buffer_views_offset(views, 2) == 80);

    // Cleanup.
    dvz_buffer_destroy(buffer);
    dvz_buffer_destroy(buffer);
    AT(dvz_buffer_handle(buffer) == VK_NULL_HANDLE);
    dvz_buffer_views_free(views);
    dvz_buffer_free(buffer);
    uint32_t err_count = dvz_gpu_ctx_error_count(ctx);
    dvz_gpu_ctx_destroy(ctx);

    return err_count > 0;
}
