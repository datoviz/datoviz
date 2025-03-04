/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Testing transfers                                                                            */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdio.h>

#include "../src/resources_utils.h"
#include "../src/transfers_utils.h"
#include "fifo.h"
#include "test.h"
#include "test_resources.h"
#include "test_transfers.h"
#include "testing.h"



/*************************************************************************************************/
/*  Test callbacks and utils                                                                     */
/*************************************************************************************************/

static DvzTransfers* get_transfers(TstSuite* suite)
{
    ANN(suite);
    DvzGpu* gpu = get_gpu(suite);
    ANN(gpu);

    DvzTransfers* transfers = calloc(1, sizeof(DvzTransfers));
    dvz_transfers(gpu, transfers);
    return transfers;
}

static void destroy_transfers(DvzTransfers* transfers)
{
    ANN(transfers);
    dvz_transfers_destroy(transfers);
    FREE(transfers);
}



static void _dl_done(DvzDeq* deq, void* item, void* user_data)
{
    if (user_data != NULL)
        *((int*)user_data) = 42;
}

static void _up_done(DvzDeq* deq, void* item, void* user_data)
{
    DvzTransferUploadDone* up = (DvzTransferUploadDone*)item;
    ANN(up);
    ANN(up->user_data);
    *((int*)up->user_data) = 314;
}



/*************************************************************************************************/
/*  Transfer tests                                                                               */
/*************************************************************************************************/

int test_transfers_buffer_mappable(TstSuite* suite, TstItem* tstitem)
{
    ANN(suite);

    DvzTransfers* transfers = get_transfers(suite);
    ANN(transfers);

    DvzGpu* gpu = transfers->gpu;
    ANN(gpu);

    // Callback for when the download has finished.
    int res = 0; // should be set to 42 by _dl_done().
    dvz_deq_callback(
        transfers->deq, DVZ_TRANSFER_DEQ_EV, DVZ_TRANSFER_DOWNLOAD_DONE, _dl_done, &res);

    uint8_t data[128] = {0};
    for (uint32_t i = 0; i < 128; i++)
        data[i] = i;

    // Allocate a staging buffer region.
    DvzBufferRegions stg = _standalone_buffer_regions(gpu, DVZ_BUFFER_TYPE_STAGING, 1, 1024);

    // Enqueue an upload transfer task.
    _enqueue_buffer_upload(transfers->deq, stg, 0, (DvzBufferRegions){0}, 0, 128, data, NULL);

    // NOTE: need to wait for the upload to be finished before we download the data.
    // The DL and UL are on different queues and may be processed out of order.
    // dvz_deq_wait(transfers->deq, DVZ_TRANSFER_PROC_CPY);
    dvz_deq_dequeue(transfers->deq, DVZ_TRANSFER_PROC_CPY, true);

    // Enqueue a download transfer task.
    uint8_t data2[128] = {0};
    _enqueue_buffer_download(transfers->deq, stg, 0, (DvzBufferRegions){0}, 0, 128, data2);
    AT(res == 0);
    dvz_deq_wait(transfers->deq, DVZ_TRANSFER_PROC_UD);

    // Wait until the download_done event has been raised, dequeue it, and finish the test.
    dvz_deq_dequeue(transfers->deq, DVZ_TRANSFER_PROC_EV, true);

    // Check that the copy worked.
    AT(data2[127] == 127);
    AT(memcmp(data2, data, 128) == 0);
    AT(res == 42);

    _destroy_buffer_regions(stg);
    destroy_transfers(transfers);
    return 0;
}



int test_transfers_buffer_large(TstSuite* suite, TstItem* tstitem)
{
    ANN(suite);

    DvzTransfers* transfers = get_transfers(suite);
    ANN(transfers);

    DvzGpu* gpu = transfers->gpu;
    ANN(gpu);

    uint64_t size = 32 * 1024 * 1024; // MB
    uint8_t* data = calloc(size, 1);
    data[0] = 1;
    data[size - 1] = 2;

    int res = 0; // should be set to 42 by _dl_done().
    dvz_deq_callback(
        transfers->deq, DVZ_TRANSFER_DEQ_EV, DVZ_TRANSFER_DOWNLOAD_DONE, _dl_done, &res);

    // Allocate a staging buffer region.
    DvzBufferRegions stg = _standalone_buffer_regions(gpu, DVZ_BUFFER_TYPE_STAGING, 1, size);

    // Enqueue an upload transfer task.
    _enqueue_buffer_upload(transfers->deq, stg, 0, (DvzBufferRegions){0}, 0, size, data, NULL);
    dvz_deq_dequeue(transfers->deq, DVZ_TRANSFER_PROC_CPY, true);

    // Wait for the transfer thread to process both transfer tasks.
    dvz_host_wait(gpu->host);

    // Enqueue a download transfer task.
    uint8_t* data2 = calloc(size, 1);
    _enqueue_buffer_download(transfers->deq, stg, 0, (DvzBufferRegions){0}, 0, size, data2);
    // This download task will be processed by the background transfer thread. At the end, it
    // will enqueue a DOWNLOAD_DONE task in the EV queue.
    AT(res == 0);

    // Wait until the download_done event has been raised, dequeue it, and finish the test.
    dvz_deq_dequeue(transfers->deq, DVZ_TRANSFER_PROC_EV, true);

    // Check that the copy worked.
    AT(data2[0] == 1);
    AT(data2[size - 1] == 2);
    AT(memcmp(data2, data, size) == 0); // SHOULD FAIL
    AT(res == 42);

    FREE(data);
    FREE(data2);

    _destroy_buffer_regions(stg);
    destroy_transfers(transfers);
    return 0;
}



int test_transfers_buffer_copy(TstSuite* suite, TstItem* tstitem)
{
    ANN(suite);

    DvzTransfers* transfers = get_transfers(suite);
    ANN(transfers);

    DvzGpu* gpu = transfers->gpu;
    ANN(gpu);

    // Callback for when the download has finished.
    int res = 0; // should be set to 42 by _dl_done().
    dvz_deq_callback(
        transfers->deq, DVZ_TRANSFER_DEQ_EV, DVZ_TRANSFER_DOWNLOAD_DONE, _dl_done, &res);
    // Called at the end of the upload transfer. Will modify the int pointer passed by
    // _enqueue_buffer_upload().
    dvz_deq_callback(
        transfers->deq, DVZ_TRANSFER_DEQ_EV, DVZ_TRANSFER_UPLOAD_DONE, _up_done, NULL);


    uint8_t data[128] = {0};
    for (uint32_t i = 0; i < 128; i++)
        data[i] = i;

    DvzBufferRegions stg = _standalone_buffer_regions(gpu, DVZ_BUFFER_TYPE_STAGING, 1, 1024);
    DvzBufferRegions br = _standalone_buffer_regions(gpu, DVZ_BUFFER_TYPE_VERTEX, 1, 1024);

    // Enqueue an upload transfer task.
    _enqueue_buffer_upload(transfers->deq, br, 0, stg, 0, 128, data, _create_upload_done(&res));
    // NOTE: we need to dequeue the copy proc manually, it is not done by the background thread
    // (the background thread only processes download/upload tasks).
    dvz_deq_dequeue(transfers->deq, DVZ_TRANSFER_PROC_CPY, true);
    // Wait until the upload_done event has been raised, and dequeue it.
    dvz_deq_dequeue(transfers->deq, DVZ_TRANSFER_PROC_EV, true);
    AT(res == 314);
    res = 0;


    // Enqueue a download transfer task.
    uint8_t data2[128] = {0};
    _enqueue_buffer_download(transfers->deq, br, 0, stg, 0, 128, data2);
    dvz_deq_dequeue(transfers->deq, DVZ_TRANSFER_PROC_CPY, true);

    // Wait until the download_done event has been raised, dequeue it, and finish the test.
    dvz_deq_dequeue(transfers->deq, DVZ_TRANSFER_PROC_EV, true);


    // Check that the copy worked.
    dvz_host_wait(gpu->host);
    AT(data2[127] == 127);
    AT(memcmp(data2, data, 128) == 0);
    AT(res == 42);

    dvz_buffer_destroy(stg.buffer);
    dvz_buffer_destroy(br.buffer);
    _destroy_buffer_regions(br);
    _destroy_buffer_regions(stg);
    destroy_transfers(transfers);
    return 0;
}



int test_transfers_image_buffer(TstSuite* suite, TstItem* tstitem)
{
    ANN(suite);

    DvzTransfers* transfers = get_transfers(suite);
    ANN(transfers);

    DvzGpu* gpu = transfers->gpu;
    ANN(gpu);

    uvec3 shape_full = {8, 24, 1};
    uvec3 offset = {0, 8, 0};
    uvec3 shape = {8, 8, 1};
    DvzSize size = 256; // 8*8*4
    DvzFormat format = DVZ_FORMAT_R8G8B8A8_UINT;

    // Texture data.
    uint8_t data[256] = {0};
    for (uint32_t i = 0; i < size; i++)
        data[i] = i;

    // Image.
    DvzImages* img = _standalone_image(gpu, DVZ_TEX_2D, shape_full, format);
    // Buffer.
    DvzBufferRegions stg = _standalone_buffer_regions(gpu, DVZ_BUFFER_TYPE_STAGING, 1, size);

    // Callback for when the download has finished.
    int res = 0; // should be set to 42 by _dl_done().
    dvz_deq_callback(
        transfers->deq, DVZ_TRANSFER_DEQ_EV, DVZ_TRANSFER_DOWNLOAD_DONE, _dl_done, &res);

    // Enqueue an upload transfer task.
    _enqueue_image_upload(transfers->deq, img, offset, shape, stg, 0, size, data, NULL);
    // NOTE: we need to dequeue the copy proc manually, it is not done by the background thread
    // (the background thread only processes download/upload tasks).
    dvz_deq_dequeue(transfers->deq, DVZ_TRANSFER_PROC_CPY, true);

    // NOTE: we should clear the staging buffer to avoid false positives.
    log_debug("clear staging buffer");
    void* null = calloc(size, 1);
    dvz_upload_buffer(transfers, stg, 0, size, null);
    FREE(null);

    // Enqueue a download transfer task.
    uint8_t data2[256] = {0};
    _enqueue_image_download(transfers->deq, img, offset, shape, stg, 0, size, data2);
    dvz_deq_dequeue(transfers->deq, DVZ_TRANSFER_PROC_CPY, true);

    // Wait until the download_done event has been raised, dequeue it, and finish the test.
    dvz_deq_dequeue(transfers->deq, DVZ_TRANSFER_PROC_EV, true);

    dvz_host_wait(gpu->host);

    // Check.
    AT(memcmp(data2, data, size) == 0);
    for (uint32_t i = 0; i < size; i++)
        AT(data2[i] == i);

    _destroy_buffer_regions(stg);
    _destroy_image(img);
    destroy_transfers(transfers);
    return 0;
}



/*************************************************************************************************/
/*  Test high-level transfer functions                                                           */
/*************************************************************************************************/

int test_transfers_direct_buffer(TstSuite* suite, TstItem* tstitem)
{
    ANN(suite);

    DvzTransfers* transfers = get_transfers(suite);
    ANN(transfers);

    DvzGpu* gpu = transfers->gpu;
    ANN(gpu);

    // Create a data array.
    uint8_t data[64] = {0};
    for (uint32_t i = 0; i < 64; i++)
        data[i] = i;

    DvzSize offset = 32;
    DvzSize size = 64;

    log_debug("start uploading data to buffer");

    // Allocate a vertex buffer.
    DvzBufferRegions br = _standalone_buffer_regions(gpu, DVZ_BUFFER_TYPE_VERTEX, 1, 128);
    dvz_upload_buffer(transfers, br, offset, size, data);

    log_debug("start downloading data from buffer");

    // Enqueue a download transfer task.
    uint8_t data2[64] = {0};
    dvz_download_buffer(transfers, br, offset, size, data2);

    // Check that the copy worked.
    AT(memcmp(data2, data, size) == 0);

    _destroy_buffer_regions(br);
    destroy_transfers(transfers);
    return 0;
}



int test_transfers_direct_image(TstSuite* suite, TstItem* tstitem)
{
    ANN(suite);

    DvzTransfers* transfers = get_transfers(suite);
    ANN(transfers);

    DvzGpu* gpu = transfers->gpu;
    ANN(gpu);

    uvec3 shape_full = {16, 48, 1};
    uvec3 offset = {0, 16, 0};
    uvec3 shape = {16, 16, 1};
    DvzSize size = 256 * 4;
    DvzFormat format = DVZ_FORMAT_R8G8B8A8_UINT;

    // Texture data.
    uint8_t data[1024] = {0};
    for (uint32_t i = 0; i < 1024; i++)
        data[i] = i % 256;

    DvzImages* img = _standalone_image(gpu, DVZ_TEX_2D, shape_full, format);

    log_debug("start uploading data to texture");
    dvz_upload_image(transfers, img, offset, shape, size, data);

    log_debug("start downloading data from buffer");
    uint8_t data2[1024] = {0};
    dvz_download_image(transfers, img, offset, shape, size, data2);

    // Check that the copy worked.
    AT(memcmp(data2, data, size) == 0);

    _destroy_image(img);
    destroy_transfers(transfers);
    return 0;
}



/*************************************************************************************************/
/*  Test dup transfers                                                                           */
/*************************************************************************************************/

int test_transfers_dups_util(TstSuite* suite, TstItem* tstitem)
{
    ANN(suite);

    DvzTransfers* transfers = get_transfers(suite);
    ANN(transfers);

    DvzGpu* gpu = transfers->gpu;
    ANN(gpu);

    uint32_t count = 3;
    DvzSize size = 16;
    DvzBufferRegions br = _standalone_buffer_regions(gpu, DVZ_BUFFER_TYPE_STAGING, count, size);
    DvzTransferDupItem* item = NULL;

    DvzTransferDups dups = _dups();
    DvzTransferType type = DVZ_TRANSFER_DUP_UPLOAD;
    DvzTransferDup tr = {.type = type, .br = br, .offset = 0, .size = size};

    AT(dups.count == 0);
    AT(_dups_empty(&dups));
    AT(!_dups_has(&dups, type, br, 0, size));

    _dups_append(&dups, &tr);
    AT(dups.count == 1);
    AT(_dups_get_idx(&dups, type, br, 0, size) == 0);
    AT(_dups_has(&dups, type, br, 0, size));

    br.offsets[0] = 1;
    AT(!_dups_has(&dups, type, br, 0, size));
    br.offsets[0] = 0;
    item = _dups_get(&dups, type, br, 0, size);

    for (uint32_t i = 0; i < count; i++)
    {
        AT(!_dups_is_done(&dups, item, i));
    }

    _dups_mark_done(&dups, item, 1);
    AT(!_dups_is_done(&dups, item, 0));
    AT(_dups_is_done(&dups, item, 1));
    AT(!_dups_all_done(&dups, item));

    _dups_mark_done(&dups, item, 0);
    AT(!_dups_all_done(&dups, item));

    _dups_mark_done(&dups, item, 2);
    AT(_dups_all_done(&dups, item));

    // Append another buffer region.
    DvzBufferRegions br1 = br;
    br1.offsets[0] = 4;
    tr.br = br1;
    _dups_append(&dups, &tr);
    item = _dups_get(&dups, type, br1, 0, size);
    AT(_dups_get_idx(&dups, type, br1, 0, size) == 1);
    AT(dups.count == 2);
    AT(!_dups_all_done(&dups, item));

    for (uint32_t i = 0; i < count; i++)
        _dups_mark_done(&dups, item, i);
    AT(_dups_all_done(&dups, item));

    // Remove the first buffer region.
    item = _dups_get(&dups, type, br, 0, size);
    _dups_remove(&dups, item);
    AT(dups.count == 1);
    AT(_dups_get_idx(&dups, type, br, 0, size) == UINT32_MAX);
    AT(_dups_get_idx(&dups, type, br1, 0, size) == 1);

    tr.br = br;
    _dups_append(&dups, &tr);
    item = _dups_get(&dups, type, br, 0, size);
    AT(dups.count == 2);
    AT(_dups_get_idx(&dups, type, br, 0, size) == 0);
    AT(!_dups_all_done(&dups, item));

    _destroy_buffer_regions(br);
    destroy_transfers(transfers);
    return 0;
}



int test_transfers_dups_upload(TstSuite* suite, TstItem* tstitem)
{
    ANN(suite);

    DvzTransfers* transfers = get_transfers(suite);
    ANN(transfers);

    DvzGpu* gpu = transfers->gpu;
    ANN(gpu);

    uint32_t count = 3;
    DvzSize size = 16;
    DvzBufferRegions br = _standalone_buffer_regions(gpu, DVZ_BUFFER_TYPE_STAGING, count, size);

    // Do nothing.
    dvz_transfers_frame(transfers, 0);

    // Enqueue a dup upload.
    uint8_t data = 42;
    _enqueue_dup_transfer(transfers->deq, br, 0, (DvzBufferRegions){0}, 0, sizeof(data), &data);

    // This will upload the data to buffer region #1 but not the others.
    dvz_transfers_frame(transfers, 1);

    // Download the buffer region and check that region #1 has the data.
    uint8_t* downloaded = (uint8_t*)calloc(size, 1);
    dvz_buffer_regions_download(&br, 1, 0, size, downloaded);
    AT(downloaded[0] == data);

    // This will upload the data to buffer region #2 but not the others.
    dvz_transfers_frame(transfers, 2);
    dvz_transfers_frame(transfers, 2); // twice, to check it doesn't change anything

    for (uint32_t i = 1; i < count; i++)
    {
        dvz_buffer_regions_download(&br, i, 0, size, downloaded);
        AT(downloaded[0] == data);
    }

    // Should not be empty.
    AT(!_dups_empty(&transfers->dups));

    // Last buffer is #0.
    dvz_transfers_frame(transfers, 0);

    for (uint32_t i = 0; i < count; i++)
    {
        dvz_buffer_regions_download(&br, i, 0, size, downloaded);
        AT(downloaded[0] == data);
    }

    // Now, should not be empty.
    AT(_dups_empty(&transfers->dups));

    _destroy_buffer_regions(br);
    FREE(downloaded);
    destroy_transfers(transfers);
    return 0;
}



int test_transfers_dups_copy(TstSuite* suite, TstItem* tstitem)
{
    ANN(suite);

    DvzTransfers* transfers = get_transfers(suite);
    ANN(transfers);

    DvzGpu* gpu = transfers->gpu;
    ANN(gpu);

    uint32_t count = 3;
    DvzSize size = 16;
    DvzBufferRegions br = _standalone_buffer_regions(gpu, DVZ_BUFFER_TYPE_VERTEX, count, size);
    DvzBufferRegions stg = _standalone_buffer_regions(gpu, DVZ_BUFFER_TYPE_STAGING, 1, size);

    // Do nothing.
    dvz_transfers_frame(transfers, 0);

    // Enqueue a dup copy.
    uint8_t data = 42;
    _enqueue_dup_transfer(transfers->deq, br, 0, stg, 0, sizeof(data), &data);

    // HACK: we need to wait for the upload to occur in the background thread before we can
    // continue with testing.
    dvz_deq_wait(transfers->deq, DVZ_TRANSFER_PROC_UD);

    // This will upload the data to buffer region #1 but not the others.
    dvz_transfers_frame(transfers, 1);


    uint8_t* downloaded = (uint8_t*)calloc(size * count, 1);

    // HACK: need to flatten the multi-buffer before downloading it here.
    DvzBufferRegions tmp = br;
    tmp.count = 1;
    tmp.size *= count;

    dvz_download_buffer(transfers, tmp, 0, tmp.size, downloaded);
    AT(downloaded[16 * 1] == data);
    AT(!_dups_empty(&transfers->dups));

    // This will upload the data to buffer region #2 but not the others.
    dvz_transfers_frame(transfers, 2);

    dvz_download_buffer(transfers, tmp, 0, tmp.size, downloaded);
    for (uint32_t i = 1; i < count; i++)
        AT(downloaded[16 * i] == data);
    AT(!_dups_empty(&transfers->dups));

    // Last buffer is #0.
    dvz_transfers_frame(transfers, 0);

    dvz_download_buffer(transfers, tmp, 0, tmp.size, downloaded);
    for (uint32_t i = 0; i < count; i++)
        AT(downloaded[16 * i] == data);
    AT(_dups_empty(&transfers->dups));


    _destroy_buffer_regions(br);
    _destroy_buffer_regions(stg);
    FREE(downloaded);
    destroy_transfers(transfers);
    return 0;
}
