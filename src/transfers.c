#include "../include/datoviz/transfers.h"
#include "../include/datoviz/canvas.h"
#include "../include/datoviz/context.h"
#include "../include/datoviz/fifo.h"



/*************************************************************************************************/
/*  FIFO                                                                                         */
/*************************************************************************************************/

static void _transfer_enqueue(DvzFifo* fifo, DvzTransfer transfer)
{
    ASSERT(fifo->capacity > 0);
    ASSERT(0 <= fifo->head && fifo->head < fifo->capacity);
    DvzTransfer* tr = (DvzTransfer*)calloc(1, sizeof(DvzTransfer));
    *tr = transfer;
    dvz_fifo_enqueue(fifo, tr);
}



static DvzTransfer _transfer_dequeue(DvzFifo* fifo, bool wait)
{
    DvzTransfer* item = (DvzTransfer*)dvz_fifo_dequeue(fifo, wait);
    DvzTransfer out;
    out.type = DVZ_TRANSFER_NONE;
    if (item == NULL)
        return out;
    ASSERT(item != NULL);
    out = *item;
    FREE(item);
    return out;
}



/*************************************************************************************************/
/*  Buffer transfers                                                                             */
/*************************************************************************************************/

static void _process_buffer_upload(DvzCanvas* canvas, DvzTransfer tr)
{
    ASSERT(canvas != NULL);
    DvzGpu* gpu = canvas->gpu;
    ASSERT(gpu != NULL);
    DvzContext* context = gpu->context;
    ASSERT(context != NULL);
    ASSERT(tr.type == DVZ_TRANSFER_BUFFER_UPLOAD);
    DvzBufferRegions br = tr.u.buf.regions;
    uint32_t idx = canvas->swapchain.img_idx;

    ASSERT(br.size > 0);
    ASSERT(tr.u.buf.data != NULL);
    ASSERT(tr.u.buf.size > 0);
    ASSERT(tr.u.buf.regions.buffer != VK_NULL_HANDLE);

    // Mappable uniforms. We only update the current swapchain image here.
    //
    // NOTE: mappable uniforms are expected to be updated at every frame (eg MVP)
    // so that every swapchain image gets the most up-to-date data.
    //
    // NOTE: this function must be called AFTER the next swapchain image has been acquired,
    // so that swapchain->img_idx corresponds to the image that will be rendered in the
    // current frame, AFTER the transfer tasks have completed. This ensures that the very
    // next frame will be up to date with the latest data and command buffer (if need
    // refill).
    if (br.buffer->type == DVZ_BUFFER_TYPE_UNIFORM_MAPPABLE)
    {
        // The mappable buffer must be constantly mapped.
        ASSERT(br.buffer->mmap != NULL);
        ASSERT(br.count == canvas->swapchain.img_count);
        ASSERT(idx < br.count);

        // NOTE: no need for alignment when copying a single buffer region (corresponding
        // to the current swapchain image)
        if (tr.u.buf.update_all_buffers)
        {
            for (uint32_t i = 0; i < tr.u.buf.regions.count; i++)
                dvz_buffer_upload(
                    br.buffer, br.offsets[i] + tr.u.buf.offset, tr.u.buf.size, tr.u.buf.data);
        }
        else
        {
            dvz_buffer_upload(
                br.buffer, br.offsets[idx] + tr.u.buf.offset, tr.u.buf.size, tr.u.buf.data);
        }
    }

    // Staging buffer.
    else if (br.buffer->type == DVZ_BUFFER_TYPE_STAGING)
    {
        // The staging buffer must be constantly mapped.
        ASSERT(br.buffer->mmap != NULL);
        ASSERT(br.count == 1);
        dvz_buffer_upload(
            br.buffer, br.offsets[0] + tr.u.buf.offset, tr.u.buf.size, tr.u.buf.data);
    }

    // All other (non-mappable) buffers. Require synchronization and copy on command
    // buffer.
    else
    {
        ASSERT(br.count == 1);

        // Take the staging buffer and ensure it is big enough.
        DvzBuffer* staging = staging_buffer(context, tr.u.buf.size);

        // Memcpy into the staging buffer.
        dvz_buffer_upload(staging, 0, tr.u.buf.size, tr.u.buf.data);

        // Copy from the staging buffer to the target buffer.
        _copy_buffer_from_staging(context, tr.u.buf.regions, tr.u.buf.offset, tr.u.buf.size);
    }
}



static void _process_buffer_download(DvzCanvas* canvas, DvzTransfer tr)
{
    ASSERT(canvas != NULL);
    DvzGpu* gpu = canvas->gpu;
    ASSERT(gpu != NULL);
    DvzContext* context = gpu->context;
    ASSERT(context != NULL);
    ASSERT(tr.type == DVZ_TRANSFER_BUFFER_DOWNLOAD);
    DvzBufferRegions br = tr.u.buf.regions;
    uint32_t idx = canvas->swapchain.img_idx;

    ASSERT(br.size > 0);
    ASSERT(tr.u.buf.data != NULL);
    ASSERT(tr.u.buf.size > 0);
    ASSERT(tr.u.buf.regions.buffer != VK_NULL_HANDLE);

    // Mappable uniforms. We only update the current swapchain image here.
    //
    // NOTE: mappable uniforms are expected to be updated at every frame (eg MVP)
    // so that every swapchain image gets the most up-to-date data.
    //
    // NOTE: this function must be called AFTER the next swapchain image has been acquired,
    // so that swapchain->img_idx corresponds to the image that will be rendered in the
    // current frame, AFTER the transfer tasks have completed. This ensures that the very
    // next frame will be up to date with the latest data and command buffer (if need
    // refill).
    if (br.buffer->type == DVZ_BUFFER_TYPE_UNIFORM_MAPPABLE)
    {
        // The mappable buffer must be constantly mapped.
        ASSERT(br.buffer->mmap != NULL);
        ASSERT(br.count == canvas->swapchain.img_count);
        ASSERT(idx < br.count);

        // NOTE: no need for alignment when copying a single buffer region (corresponding
        // to the current swapchain image)
        dvz_buffer_download(
            br.buffer, br.offsets[idx] + tr.u.buf.offset, tr.u.buf.size, tr.u.buf.data);
    }

    // Staging buffer.
    else if (br.buffer->type == DVZ_BUFFER_TYPE_STAGING)
    {
        // The staging buffer must be constantly mapped.
        ASSERT(br.buffer->mmap != NULL);
        ASSERT(br.count == 1);
        dvz_buffer_download(
            br.buffer, br.offsets[0] + tr.u.buf.offset, tr.u.buf.size, tr.u.buf.data);
    }

    // All other (non-mappable) buffers. Require synchronization and copy on command
    // buffer.
    else
    {
        ASSERT(br.count == 1);

        // Take the staging buffer and ensure it is big enough.
        DvzBuffer* staging = staging_buffer(context, tr.u.buf.size);

        // Copy from the source buffer to the staging buffer.
        _copy_buffer_to_staging(context, tr.u.buf.regions, tr.u.buf.offset, tr.u.buf.size);

        // Memcpy into the staging buffer.
        dvz_buffer_download(staging, 0, tr.u.buf.size, tr.u.buf.data);
    }
}



static void _process_buffer_copy(DvzCanvas* canvas, DvzTransfer tr)
{
    ASSERT(canvas != NULL);
    DvzGpu* gpu = canvas->gpu;
    ASSERT(gpu != NULL);
    DvzContext* context = gpu->context;
    ASSERT(context != NULL);
    ASSERT(tr.type == DVZ_TRANSFER_BUFFER_COPY);

    DvzBufferRegions* src = &tr.u.buf_copy.src;
    DvzBufferRegions* dst = &tr.u.buf_copy.dst;
    ASSERT(src->count == dst->count);

    VkDeviceSize size = tr.u.buf_copy.size;
    VkDeviceSize src_offset = tr.u.buf_copy.src_offset;
    VkDeviceSize dst_offset = tr.u.buf_copy.dst_offset;

    // Take transfer cmd buf.
    DvzCommands* cmds = &context->transfer_cmd;
    dvz_cmd_reset(cmds, 0);
    dvz_cmd_begin(cmds, 0);

    // Copy buffer command.
    VkBufferCopy* regions = (VkBufferCopy*)calloc(src->count, sizeof(VkBufferCopy));
    for (uint32_t i = 0; i < src->count; i++)
    {
        regions[i].size = size;
        regions[i].srcOffset = src->offsets[i] + src_offset;
        regions[i].dstOffset = dst->offsets[i] + dst_offset;
    }
    vkCmdCopyBuffer(cmds->cmds[0], src->buffer->buffer, dst->buffer->buffer, src->count, regions);

    dvz_cmd_end(cmds, 0);
    FREE(regions);

    // Wait for the render queue to be idle.
    dvz_queue_wait(gpu, DVZ_DEFAULT_QUEUE_RENDER);

    // Submit the commands to the transfer queue.
    DvzSubmit submit = dvz_submit(gpu);
    dvz_submit_commands(&submit, cmds);
    log_debug("copy %s between 2 buffers", pretty_size(size));
    dvz_submit_send(&submit, 0, NULL, 0);

    // Wait for the transfer queue to be idle.
    dvz_queue_wait(gpu, DVZ_DEFAULT_QUEUE_TRANSFER);
}



/*************************************************************************************************/
/*  Canvas transfers processing                                                                  */
/*************************************************************************************************/

void dvz_process_transfers(DvzCanvas* canvas)
{
    // This function is to be called at every frame, after the FRAME callbacks (so that FRAME
    // callbacks calling dvz_upload_buffers() have their transfers processed immediately in the
    // same frame), but before queue submit, so that we may get a chance to ask for a command
    // buffer refill before submission (if a transfer requires a refill, e.g. after a vertex buffer
    // count change)
    ASSERT(canvas != NULL);
    DvzGpu* gpu = canvas->gpu;
    ASSERT(gpu != NULL);
    DvzContext* context = gpu->context;
    ASSERT(context != NULL);
    DvzFifo* fifo = &canvas->transfers;
    // Do nothing if there are no pending transfers.
    if (fifo->is_empty)
        return;

    // Process all pending transfer tasks.
    DvzTransfer tr = {0};
    while (true)
    {
        tr = _transfer_dequeue(fifo, false);
        if (tr.type == DVZ_TRANSFER_NONE)
            break;
        fifo->is_processing = true;

        // Process buffer transfers.
        if (tr.type == DVZ_TRANSFER_BUFFER_UPLOAD)
            _process_buffer_upload(canvas, tr);
        if (tr.type == DVZ_TRANSFER_BUFFER_DOWNLOAD)
            _process_buffer_download(canvas, tr);
        if (tr.type == DVZ_TRANSFER_BUFFER_COPY)
            _process_buffer_copy(canvas, tr);

        // Process texture transfers.
        if (tr.type == DVZ_TRANSFER_TEXTURE_UPLOAD)
            dvz_texture_upload(
                tr.u.tex.texture, tr.u.tex.offset, tr.u.tex.shape, tr.u.tex.size, tr.u.tex.data);
        if (tr.type == DVZ_TRANSFER_TEXTURE_DOWNLOAD)
            dvz_texture_download(
                tr.u.tex.texture, tr.u.tex.offset, tr.u.tex.shape, tr.u.tex.size, tr.u.tex.data);
        if (tr.type == DVZ_TRANSFER_TEXTURE_COPY)
            dvz_texture_copy(
                tr.u.tex_copy.src, tr.u.tex_copy.src_offset, tr.u.tex_copy.dst,
                tr.u.tex_copy.dst_offset, tr.u.tex_copy.shape);

        fifo->is_processing = false;
    }
}



/*************************************************************************************************/
/*  Canvas buffer transfers                                                                      */
/*************************************************************************************************/

static void _enqueue_buffers_transfer(
    DvzCanvas* canvas, DvzDataTransferType type, DvzBufferRegions br, //
    VkDeviceSize offset, VkDeviceSize size, void* data)
{
    ASSERT(canvas != NULL);
    ASSERT(canvas->gpu != NULL);
    DvzContext* context = canvas->gpu->context;
    ASSERT(context != NULL);
    ASSERT(canvas->transfers.capacity > 0);
    ASSERT(size > 0);
    ASSERT(br.buffer != NULL);
    ASSERT(dvz_obj_is_created(&br.buffer->obj));
    ASSERT(data != NULL);

    // Create the transfer object.
    DvzTransfer tr = {0};
    tr.type = type;
    tr.u.buf.regions = br;
    tr.u.buf.offset = offset;
    tr.u.buf.size = size;
    tr.u.buf.data = data;

    // HACK: when uploading buffers when the app is not running (for example at initialization)
    // we upload all copies of the DvzBufferRegions. This is used when using UNIFORM_MAPPABLE
    // buffers that are not continuously updated in each frame.
    tr.u.buf.update_all_buffers = !canvas->app->is_running;

    _transfer_enqueue(&canvas->transfers, tr);
}



// WARNING: these functions require that the pointer lives through the next frame (no copy)
void dvz_upload_buffers(
    DvzCanvas* canvas, DvzBufferRegions br, VkDeviceSize offset, VkDeviceSize size, void* data)
{
    _enqueue_buffers_transfer(canvas, DVZ_TRANSFER_BUFFER_UPLOAD, br, offset, size, data);

    if (!canvas->app->is_running)
        dvz_process_transfers(canvas);
}



void dvz_download_buffers(
    DvzCanvas* canvas, DvzBufferRegions br, VkDeviceSize offset, VkDeviceSize size, void* data)
{
    _enqueue_buffers_transfer(canvas, DVZ_TRANSFER_BUFFER_DOWNLOAD, br, offset, size, data);

    if (!canvas->app->is_running)
        dvz_process_transfers(canvas);
}



void dvz_copy_buffers(
    DvzCanvas* canvas, DvzBufferRegions src, VkDeviceSize src_offset, //
    DvzBufferRegions dst, VkDeviceSize dst_offset, VkDeviceSize size)
{
    ASSERT(canvas != NULL);
    ASSERT(canvas->transfers.capacity > 0);
    ASSERT(size > 0);
    ASSERT(src.buffer != NULL);
    ASSERT(dst.buffer != NULL);
    ASSERT(canvas->gpu != NULL);
    DvzContext* context = canvas->gpu->context;
    ASSERT(context != NULL);

    // Create the transfer object.
    DvzTransfer tr = {0};
    tr.type = DVZ_TRANSFER_BUFFER_COPY;
    tr.u.buf_copy.src = src;
    tr.u.buf_copy.dst = dst;
    tr.u.buf_copy.src_offset = src_offset;
    tr.u.buf_copy.dst_offset = dst_offset;
    tr.u.buf_copy.size = size;

    _transfer_enqueue(&canvas->transfers, tr);

    if (!canvas->app->is_running)
        dvz_process_transfers(canvas);
}



/*************************************************************************************************/
/*  Canvas texture transfers                                                                     */
/*************************************************************************************************/

static void _enqueue_texture_transfer(
    DvzCanvas* canvas, DvzDataTransferType type, DvzTexture* texture, //
    uvec3 offset, uvec3 shape, VkDeviceSize size, void* data)
{
    ASSERT(canvas != NULL);
    ASSERT(canvas->gpu != NULL);
    DvzContext* context = canvas->gpu->context;
    ASSERT(context != NULL);
    ASSERT(canvas->transfers.capacity > 0);
    ASSERT(texture != NULL);
    ASSERT(dvz_obj_is_created(&texture->obj));
    ASSERT(size > 0);
    ASSERT(data != NULL);

    if (shape[0] == 0)
        shape[0] = texture->image->width;
    if (shape[1] == 0)
        shape[1] = texture->image->height;
    if (shape[2] == 0)
        shape[2] = texture->image->depth;

    // Create the transfer object.
    DvzTransfer tr = {0};
    tr.type = type;
    for (uint32_t i = 0; i < 3; i++)
    {
        tr.u.tex.shape[i] = shape[i];
        tr.u.tex.offset[i] = offset[i];
    }
    tr.u.tex.size = size;
    tr.u.tex.data = data;
    tr.u.tex.texture = texture;

    _transfer_enqueue(&canvas->transfers, tr);
}



void dvz_upload_texture(
    DvzCanvas* canvas, DvzTexture* texture, uvec3 offset, uvec3 shape, VkDeviceSize size,
    void* data)
{
    _enqueue_texture_transfer(
        canvas, DVZ_TRANSFER_TEXTURE_UPLOAD, texture, offset, shape, size, data);

    if (!canvas->app->is_running)
        dvz_process_transfers(canvas);
}



void dvz_download_texture(
    DvzCanvas* canvas, DvzTexture* texture, uvec3 offset, uvec3 shape, VkDeviceSize size,
    void* data)
{
    _enqueue_texture_transfer(
        canvas, DVZ_TRANSFER_TEXTURE_DOWNLOAD, texture, offset, shape, size, data);

    if (!canvas->app->is_running)
        dvz_process_transfers(canvas);
}



void dvz_copy_texture(
    DvzCanvas* canvas, DvzTexture* src, uvec3 src_offset, DvzTexture* dst, uvec3 dst_offset,
    uvec3 shape, VkDeviceSize size)
{
    ASSERT(canvas != NULL);
    ASSERT(canvas->gpu != NULL);
    DvzContext* context = canvas->gpu->context;
    ASSERT(context != NULL);
    ASSERT(canvas->transfers.capacity > 0);
    ASSERT(src != NULL);
    ASSERT(dvz_obj_is_created(&src->obj));
    ASSERT(dst != NULL);
    ASSERT(dvz_obj_is_created(&dst->obj));
    ASSERT(size > 0);

    // Create the transfer object.
    DvzTransfer tr = {0};
    tr.type = DVZ_TRANSFER_TEXTURE_COPY;
    tr.u.tex_copy.src = src;
    tr.u.tex_copy.dst = dst;
    memcpy(tr.u.tex_copy.src_offset, src_offset, sizeof(uvec3));
    memcpy(tr.u.tex_copy.dst_offset, dst_offset, sizeof(uvec3));
    memcpy(tr.u.tex_copy.shape, shape, sizeof(uvec3));

    _transfer_enqueue(&canvas->transfers, tr);

    if (!canvas->app->is_running)
        dvz_process_transfers(canvas);
}
