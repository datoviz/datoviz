/*************************************************************************************************/
/*  GPU data transfers                                                                           */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

// #include "../include/datoviz/canvas.h"
#include "transfers.h"
#include "fifo.h"
#include "resources_utils.h"
#include "transfers_utils.h"
#include "window.h"



/*************************************************************************************************/
/*  Utils                                                                                        */
/*************************************************************************************************/

// Process for the deq proc #0, which encompasses the two queues UPLOAD and DOWNLOAD.
static void* _thread_transfers(void* user_data)
{
    DvzTransfers* transfers = (DvzTransfers*)user_data;
    ASSERT(transfers != NULL);
    dvz_deq_dequeue_loop(&transfers->deq, DVZ_TRANSFER_PROC_UD);
    return NULL;
}



static void _create_transfers(DvzTransfers* transfers)
{
    ASSERT(transfers != NULL);
    transfers->deq = dvz_deq(5);

    // Producer/consumer pairs (deq processes).
    dvz_deq_proc(
        &transfers->deq, DVZ_TRANSFER_PROC_UD, //
        2, (uint32_t[]){DVZ_TRANSFER_DEQ_UL, DVZ_TRANSFER_DEQ_DL});
    dvz_deq_proc(
        &transfers->deq, DVZ_TRANSFER_PROC_CPY, //
        1, (uint32_t[]){DVZ_TRANSFER_DEQ_COPY});
    dvz_deq_proc(
        &transfers->deq, DVZ_TRANSFER_PROC_EV, //
        1, (uint32_t[]){DVZ_TRANSFER_DEQ_EV});
    dvz_deq_proc(
        &transfers->deq, DVZ_TRANSFER_PROC_DUP, //
        1, (uint32_t[]){DVZ_TRANSFER_DEQ_DUP});

    // Transfer deq callbacks.

    // Uploads.
    dvz_deq_callback(
        &transfers->deq, DVZ_TRANSFER_DEQ_UL, //
        DVZ_TRANSFER_BUFFER_UPLOAD,           //
        _process_buffer_upload, transfers);

    // Uploads on the main thread, used when there is no staging buffer.
    dvz_deq_callback(
        &transfers->deq, DVZ_TRANSFER_DEQ_COPY, //
        DVZ_TRANSFER_BUFFER_UPLOAD,             //
        _process_buffer_upload, transfers);

    // Downloads.
    dvz_deq_callback(
        &transfers->deq, DVZ_TRANSFER_DEQ_DL, //
        DVZ_TRANSFER_BUFFER_DOWNLOAD,         //
        _process_buffer_download, transfers);

    // Copies.
    dvz_deq_callback(
        &transfers->deq, DVZ_TRANSFER_DEQ_COPY, //
        DVZ_TRANSFER_BUFFER_COPY,               //
        _process_buffer_copy, transfers);

    dvz_deq_callback(
        &transfers->deq, DVZ_TRANSFER_DEQ_COPY, //
        DVZ_TRANSFER_IMAGE_COPY,                //
        _process_image_copy, transfers);

    // Buffer/image copies.
    dvz_deq_callback(
        &transfers->deq, DVZ_TRANSFER_DEQ_COPY, //
        DVZ_TRANSFER_IMAGE_BUFFER,              //
        _process_image_buffer, transfers);

    dvz_deq_callback(
        &transfers->deq, DVZ_TRANSFER_DEQ_COPY, //
        DVZ_TRANSFER_BUFFER_IMAGE,              //
        _process_buffer_image, transfers);

    // Transfer thread.
    transfers->thread = dvz_thread(_thread_transfers, transfers);

    // Transfer dups
    transfers->dups = _dups();

    dvz_deq_callback(
        &transfers->deq, DVZ_TRANSFER_DEQ_DUP, //
        DVZ_TRANSFER_DUP_UPLOAD,               //
        _append_dup_item, transfers);

    dvz_deq_callback(
        &transfers->deq, DVZ_TRANSFER_DEQ_DUP, //
        DVZ_TRANSFER_DUP_COPY,                 //
        _append_dup_item, transfers);
}



static void
_process_pending_dup(DvzTransfers* transfers, DvzTransferDupItem* item, uint32_t img_idx)
{
    ASSERT(transfers != NULL);
    ASSERT(item != NULL);

    DvzGpu* gpu = transfers->gpu;
    ASSERT(gpu != NULL);

    DvzBufferRegions* br = &item->tr.br;
    ASSERT(img_idx < br->count);
    bool recurrent = item->tr.recurrent;

    // if the current buffer region is marked as done, stop immediately.
    if (!recurrent && _dups_is_done(&transfers->dups, item, img_idx))
    {
        log_debug("skip dup transfer processing for image #%d", img_idx);
        return;
    }

    log_debug("processing dup transfer for image #%d", img_idx);

    // Dup upload.
    if (item->tr.type == DVZ_TRANSFER_DUP_UPLOAD)
    {
        log_debug("direct mappable upload");

        // Upload the data directly (safe when this function is properly called in the event loop)
        dvz_buffer_regions_upload(br, img_idx, item->tr.offset, item->tr.size, item->tr.data);
    }
    // Dup copy.
    else if (item->tr.type == DVZ_TRANSFER_DUP_COPY)
    {
        log_debug("copy from staging to buffer, region #%d", img_idx);

        // NOTE: the staging buffer only need to have 1 copy (1 region) of the data. We copy it to
        // region #img_idx of the target buffer.
        ASSERT(item->tr.stg.count == 1);

        // Submit a copy for the img_idx part, from staging to target buffer, and wait.
        dvz_buffer_regions_copy(
            &item->tr.stg, 0, item->tr.stg_offset, br, img_idx, item->tr.offset, item->tr.size);

        // NOTE: is the wait really necessary here? we could also use a fence, or not wait at all?
        dvz_queue_wait(gpu, DVZ_DEFAULT_QUEUE_TRANSFER);
    }
    else
    {
        log_error("unknown transfer dup type");
    }

    if (!recurrent)
    {
        // Mark this region as done
        _dups_mark_done(&transfers->dups, item, img_idx);

        // If all regions are done, remove the current item from the list.
        if (_dups_all_done(&transfers->dups, item))
            _dups_remove(&transfers->dups, item);
    }
}



/*************************************************************************************************/
/*  Transfers struct                                                                             */
/*************************************************************************************************/

void dvz_transfers(DvzGpu* gpu, DvzTransfers* transfers)
{

    ASSERT(gpu != NULL);
    ASSERT(dvz_obj_is_created(&gpu->obj));
    ASSERT(transfers != NULL);
    ASSERT(!dvz_obj_is_created(&transfers->obj));
    // NOTE: this function should only be called once, at context creation.

    log_trace("creating transfers");

    // Create the resources.
    transfers->gpu = gpu;
    _create_transfers(transfers);

    dvz_obj_created(&transfers->obj);
}



// This function is meant to be called at every frame by the event loop, in a FRAME canvas callback
// running in MAIN queue (main thread).
void dvz_transfers_frame(DvzTransfers* transfers, uint32_t img_idx)
{
    ASSERT(transfers != NULL);
    log_trace("transfers frame #%d", img_idx);

    DvzGpu* gpu = transfers->gpu;
    ASSERT(gpu != NULL);

    // Dequeue all pending copies (which are either buffer copies, or direct mappable).
    // This is NOT used for transfer dups, which are enqueued in a different queue/proc (DUP).
    // NOTE: this call *blocks* the GPU until the copies are complete.
    dvz_deq_dequeue_batch(&transfers->deq, DVZ_TRANSFER_PROC_CPY);

    // Dequeue the pending EV items, mostly used for UPLOAD_DONE events (temporary staging dat
    // deallocation).
    dvz_deq_dequeue_batch(&transfers->deq, DVZ_TRANSFER_PROC_EV);

    // Now, process dup transfers.
    dvz_deq_dequeue_batch(&transfers->deq, DVZ_TRANSFER_PROC_DUP);

    // Check if there are ongoing non-recurrent dup transfers.
    DvzTransferDups* dups = &transfers->dups;
    if (_dups_empty(dups))
    {
        log_trace("no ongoing dup transfer");
        return;
    }
    // HACK: should be wrapped in an interface instead.
    // Process all ongoing dups.
    DvzTransferDupItem* item = NULL;
    for (uint32_t i = 0; i < DVZ_DUPS_MAX; i++)
    {
        item = &dups->dups[i];
        ASSERT(item != NULL);
        if (item->is_set)
        {
            _process_pending_dup(transfers, item, img_idx);
        }
    }
}



void dvz_transfers_destroy(DvzTransfers* transfers)
{
    if (transfers == NULL)
    {
        log_error("skip destruction of null transfers");
        return;
    }
    log_trace("destroying transfers");
    ASSERT(transfers != NULL);
    ASSERT(transfers->gpu != NULL);

    // Enqueue a STOP task to stop the UL and DL threads.
    dvz_deq_enqueue(&transfers->deq, DVZ_TRANSFER_DEQ_UL, 0, NULL);
    dvz_deq_enqueue(&transfers->deq, DVZ_TRANSFER_DEQ_DL, 0, NULL);

    // Join the UL and DL threads.
    dvz_thread_join(&transfers->thread);

    // Destroy the deq.
    dvz_deq_destroy(&transfers->deq);

    // Mark the object as destroyed.
    dvz_obj_destroyed(&transfers->obj);
}



/*************************************************************************************************/
/*  Buffer transfers                                                                             */
/*************************************************************************************************/

// WARNING: do not use the functions below except for offscreen/testing purposes.

static void _flush_transfers(DvzTransfers* transfers)
{
    ASSERT(transfers != NULL);
    ASSERT(transfers->gpu != NULL);
    ASSERT(transfers->gpu->host != NULL);

    // Flush all queues.
    for (uint32_t i = 0; i < transfers->deq.proc_count; i++)
    {
        log_debug("flush transfers deq #%d", i);
        dvz_deq_dequeue_batch(&transfers->deq, i);
    }
    dvz_host_wait(transfers->gpu->host);
}



void dvz_upload_buffer(
    DvzTransfers* transfers, DvzBufferRegions br, //
    VkDeviceSize offset, VkDeviceSize size, void* data)
{
    ASSERT(transfers != NULL);
    ASSERT(br.buffer != NULL);
    ASSERT(data != NULL);
    ASSERT(size > 0);
    ASSERT(br.count == 1);

    DvzGpu* gpu = transfers->gpu;
    ASSERT(gpu != NULL);

    log_debug("upload %s to a buffer", pretty_size(size));

    _flush_transfers(transfers);

    // NOTE: not optimal at all: we create a special staging DvzBuffer and we delete it at the end.
    // Furthermore, we could avoid using a staging buffer by testing if the buffer is host-visible.
    DvzBufferRegions stg = _standalone_buffer_regions(gpu, DVZ_BUFFER_TYPE_STAGING, 1, size);

    // Enqueue an upload transfer task.
    _enqueue_buffer_upload(&transfers->deq, br, offset, stg, 0, size, data, NULL);
    // NOTE: we need to dequeue the copy proc manually, it is not done by the background thread
    // (the background thread only processes download/upload tasks).
    dvz_deq_dequeue(&transfers->deq, DVZ_TRANSFER_PROC_CPY, true);
    dvz_deq_wait(&transfers->deq, DVZ_TRANSFER_PROC_UD);

    // Destroy the transient staging buffer.
    _destroy_buffer_regions(stg);
}



void dvz_download_buffer(
    DvzTransfers* transfers, DvzBufferRegions br, //
    VkDeviceSize offset, VkDeviceSize size, void* data)
{
    ASSERT(transfers != NULL);
    ASSERT(br.buffer != NULL);
    ASSERT(data != NULL);
    ASSERT(size > 0);
    ASSERT(br.count == 1);

    DvzGpu* gpu = transfers->gpu;
    ASSERT(gpu != NULL);

    log_debug("download %s from a buffer", pretty_size(size));

    _flush_transfers(transfers);

    // NOTE: not optimal at all: we create a special staging DvzBuffer and we delete it at the end.
    // Furthermore, we could avoid using a staging buffer by testing if the buffer is host-visible.
    DvzBufferRegions stg = _standalone_buffer_regions(gpu, DVZ_BUFFER_TYPE_STAGING, 1, size);

    // Enqueue an upload transfer task.
    _enqueue_buffer_download(&transfers->deq, br, offset, stg, 0, size, data);
    // NOTE: we need to dequeue the copy proc manually, it is not done by the background thread
    // (the background thread only processes download/upload tasks).
    dvz_deq_dequeue(&transfers->deq, DVZ_TRANSFER_PROC_CPY, true);
    dvz_deq_wait(&transfers->deq, DVZ_TRANSFER_PROC_UD);

    // Wait until the download is done.
    dvz_deq_dequeue(&transfers->deq, DVZ_TRANSFER_PROC_EV, true);
    dvz_deq_wait(&transfers->deq, DVZ_TRANSFER_PROC_EV);

    // Destroy the transient staging buffer.
    _destroy_buffer_regions(stg);
}



void dvz_copy_buffer(
    DvzTransfers* transfers,                       //
    DvzBufferRegions src, VkDeviceSize src_offset, //
    DvzBufferRegions dst, VkDeviceSize dst_offset, //
    VkDeviceSize size)
{
    ASSERT(transfers != NULL);
    ASSERT(src.buffer != NULL);
    ASSERT(dst.buffer != NULL);
    ASSERT(src.count == 1);
    ASSERT(dst.count == 1);
    ASSERT(size > 0);

    log_debug("copy %s between buffers", pretty_size(size));

    _flush_transfers(transfers);

    // Enqueue an upload transfer task.
    _enqueue_buffer_copy(&transfers->deq, src, src_offset, dst, dst_offset, size);
    // NOTE: we need to dequeue the copy proc manually, it is not done by the background thread
    // (the background thread only processes download/upload tasks).
    dvz_deq_dequeue(&transfers->deq, DVZ_TRANSFER_PROC_CPY, true);
    dvz_deq_wait(&transfers->deq, DVZ_TRANSFER_PROC_UD);
}



/*************************************************************************************************/
/*  Images transfers                                                                            */
/*************************************************************************************************/

static void _full_tex_shape(DvzImages* img, uvec3 shape)
{
    ASSERT(img != NULL);
    for (uint32_t i = 0; i < 3; i++)
        if (shape[i] == 0)
            shape[i] = img->shape[i];
}



void dvz_upload_image(
    DvzTransfers* transfers, DvzImages* img, //
    uvec3 offset, uvec3 shape, VkDeviceSize size, void* data)
{
    ASSERT(transfers != NULL);
    ASSERT(img != NULL);
    ASSERT(data != NULL);
    ASSERT(size > 0);
    ASSERT(img->count == 1);

    _flush_transfers(transfers);

    DvzGpu* gpu = transfers->gpu;
    ASSERT(gpu != NULL);

    _full_tex_shape(img, shape);
    ASSERT(shape[0] > 0);
    ASSERT(shape[1] > 0);
    ASSERT(shape[2] > 0);

    // NOTE: not optimal at all: we create a special staging DvzBuffer and we delete it at the end.
    // Furthermore, we could avoid using a staging buffer by testing if the buffer is host-visible.
    DvzBufferRegions stg = _standalone_buffer_regions(gpu, DVZ_BUFFER_TYPE_STAGING, 1, size);
    _enqueue_image_upload(&transfers->deq, img, offset, shape, stg, 0, size, data);

    // Destroy the transient staging buffer.
    dvz_deq_dequeue(&transfers->deq, DVZ_TRANSFER_PROC_CPY, true);
    dvz_deq_wait(&transfers->deq, DVZ_TRANSFER_PROC_UD);

    _destroy_buffer_regions(stg);
}



void dvz_download_image(
    DvzTransfers* transfers, DvzImages* img, //
    uvec3 offset, uvec3 shape, VkDeviceSize size, void* data)
{
    ASSERT(transfers != NULL);
    ASSERT(img != NULL);
    ASSERT(data != NULL);
    ASSERT(size > 0);
    if (img->count > 1)
    {
        log_debug("note: downloading a single copy of a multiset image");
    }

    _flush_transfers(transfers);

    DvzGpu* gpu = transfers->gpu;
    ASSERT(gpu != NULL);

    _full_tex_shape(img, shape);
    ASSERT(shape[0] > 0);
    ASSERT(shape[1] > 0);
    ASSERT(shape[2] > 0);

    // NOTE: not optimal at all: we create a special staging DvzBuffer and we delete it at the end.
    // Furthermore, we could avoid using a staging buffer by testing if the buffer is host-visible.
    DvzBufferRegions stg = _standalone_buffer_regions(gpu, DVZ_BUFFER_TYPE_STAGING, 1, size);

    _enqueue_image_download(&transfers->deq, img, offset, shape, stg, 0, size, data);

    dvz_deq_dequeue(&transfers->deq, DVZ_TRANSFER_PROC_CPY, true);
    dvz_deq_wait(&transfers->deq, DVZ_TRANSFER_PROC_UD);

    // Wait until the download is done.
    // dvz_deq_dequeue(&transfers->deq, DVZ_TRANSFER_PROC_EV, true);
    dvz_deq_dequeue_batch(&transfers->deq, DVZ_TRANSFER_PROC_EV);

    // Destroy the transient staging buffer.
    _destroy_buffer_regions(stg);
}



void dvz_copy_image(
    DvzTransfers* transfers,          //
    DvzImages* src, uvec3 src_offset, //
    DvzImages* dst, uvec3 dst_offset, //
    uvec3 shape, VkDeviceSize size)
{
    ASSERT(transfers != NULL);
    ASSERT(src != NULL);
    ASSERT(dst != NULL);
    ASSERT(src->count == 1);
    ASSERT(dst->count == 1);

    log_debug("copy %s between images", pretty_size(size));

    _flush_transfers(transfers);

    _enqueue_image_copy(&transfers->deq, src, src_offset, dst, dst_offset, shape);

    dvz_deq_dequeue(&transfers->deq, DVZ_TRANSFER_PROC_CPY, true);
    dvz_deq_wait(&transfers->deq, DVZ_TRANSFER_PROC_UD);
}
