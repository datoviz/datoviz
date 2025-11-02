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

#include "../../vk/tests/test_vk.h"
#include "../types.h"
#include "_assertions.h"
#include "_compat.h"
#include "datoviz/vk/bootstrap.h"
#include "datoviz/vklite/buffers.h"
#include "test_vklite.h"



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
    DvzBootstrap bootstrap = {0};
    dvz_bootstrap(&bootstrap, 0);

    DvzBuffer buffer = {0};
    DvzSize size = 65536;

    dvz_buffer(&bootstrap.device, &bootstrap.allocator, &buffer);
    dvz_buffer_size(&buffer, size);
    dvz_buffer_flags(&buffer, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT);
    dvz_buffer_usage(&buffer, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
    dvz_buffer_create(&buffer);

    // Map the buffer.
    dvz_buffer_map(&buffer);

    // Create some data.
    uint8_t data[MAP_SIZE] = {0};
    DvzSize msize = MAP_SIZE;
    for (uint32_t i = 0; i < MAP_SIZE; i++)
    {
        data[i] = i;
    }
    DvzSize offset = MAP_OFFSET;

    // Upload the data and check.
    dvz_buffer_upload(&buffer, offset, msize, data);
    AT(data[10] == 10);

    // Reset the data.
    dvz_buffer_unmap(&buffer);
    dvz_memset(data, msize, 0, msize);
    AT(data[10] == 0);

    // Download the data and check again.
    dvz_buffer_download(&buffer, offset, msize, data);
    AT(data[10] == 10);


    // RESIZING.

    // No-op as buffer is smaller.
    dvz_buffer_resize(&buffer, size / 2);

    // Download the data and check again.
    dvz_buffer_download(&buffer, offset, msize, data);
    AT(data[10] == 10);

    VkBuffer vk_buffer = buffer.vk_buffer;

    // Buffer recreated if size is larger.
    dvz_buffer_resize(&buffer, 2 * size);

    // Download the data and check again.
    dvz_buffer_download(&buffer, offset, msize, data);
    AT(buffer.alloc.info.size == 2 * size);

    AT(buffer.vk_buffer != vk_buffer);

    // Cleanup.
    dvz_buffer_destroy(&buffer);
    dvz_bootstrap_destroy(&bootstrap);

    RETURN_VALIDATION
}



int test_vklite_buffer_views(TstSuite* suite, TstItem* tstitem)
{
    // Bootstrap.
    DvzBootstrap bootstrap = {0};
    dvz_bootstrap(&bootstrap, 0);

    DvzBuffer buffer = {0};
    DvzSize size = 65536;

    dvz_buffer(&bootstrap.device, &bootstrap.allocator, &buffer);
    dvz_buffer_size(&buffer, size);
    dvz_buffer_flags(&buffer, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT);
    dvz_buffer_usage(&buffer, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
    dvz_buffer_create(&buffer);

    DvzBufferViews views = {0};
    DvzSize offset = 33;
    DvzSize vsize = 7;
    DvzSize alignment = 16;
    dvz_buffer_views(&buffer, 3, offset, vsize, alignment, &views);
    AT(views.offsets[0] == 48);
    AT(views.offsets[1] == 64);
    AT(views.offsets[2] == 80);

    // Cleanup.
    dvz_buffer_destroy(&buffer);
    dvz_bootstrap_destroy(&bootstrap);

    RETURN_VALIDATION
}
