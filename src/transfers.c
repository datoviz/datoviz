#include "../include/visky/transfers.h"
#include "../include/visky/canvas.h"
#include "../include/visky/context.h"
#include "../include/visky/fifo.h"



/*************************************************************************************************/
/*  Buffer transfers                                                                             */
/*************************************************************************************************/

static void _process_buffer_upload(VklCanvas* canvas, VklTransfer tr)
{
    ASSERT(canvas != NULL);
    VklGpu* gpu = canvas->gpu;
    ASSERT(gpu != NULL);
    VklContext* context = canvas->gpu->context;
    ASSERT(tr.type == VKL_TRANSFER_BUFFER_UPLOAD);
    VklBufferRegions br = tr.u.buf.regions;
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
    if (br.buffer->type == VKL_BUFFER_TYPE_UNIFORM_MAPPABLE)
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
                vkl_buffer_upload(
                    br.buffer, br.offsets[i] + tr.u.buf.offset, tr.u.buf.size, tr.u.buf.data);
        }
        else
        {
            vkl_buffer_upload(
                br.buffer, br.offsets[idx] + tr.u.buf.offset, tr.u.buf.size, tr.u.buf.data);
        }
    }

    // Staging buffer.
    else if (br.buffer->type == VKL_BUFFER_TYPE_STAGING)
    {
        // The staging buffer must be constantly mapped.
        ASSERT(br.buffer->mmap != NULL);
        ASSERT(br.count == 1);
        vkl_buffer_upload(
            br.buffer, br.offsets[0] + tr.u.buf.offset, tr.u.buf.size, tr.u.buf.data);
    }

    // All other (non-mappable) buffers. Require synchronization and copy on command
    // buffer.
    else
    {
        ASSERT(br.count == 1);

        // Take the staging buffer and ensure it is big enough.
        VklBuffer* staging = staging_buffer(context, tr.u.buf.size);

        // Memcpy into the staging buffer.
        vkl_buffer_upload(staging, 0, tr.u.buf.size, tr.u.buf.data);

        // Copy from the staging buffer to the target buffer.
        _copy_buffer_from_staging(context, tr.u.buf.regions, tr.u.buf.offset, tr.u.buf.size);
    }
}



static void _process_buffer_download(VklCanvas* canvas, VklTransfer tr)
{
    ASSERT(canvas != NULL);
    VklGpu* gpu = canvas->gpu;
    ASSERT(gpu != NULL);
    VklContext* context = canvas->gpu->context;
    ASSERT(tr.type == VKL_TRANSFER_BUFFER_DOWNLOAD);
    VklBufferRegions br = tr.u.buf.regions;
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
    if (br.buffer->type == VKL_BUFFER_TYPE_UNIFORM_MAPPABLE)
    {
        // The mappable buffer must be constantly mapped.
        ASSERT(br.buffer->mmap != NULL);
        ASSERT(br.count == canvas->swapchain.img_count);
        ASSERT(idx < br.count);

        // NOTE: no need for alignment when copying a single buffer region (corresponding
        // to the current swapchain image)
        vkl_buffer_download(
            br.buffer, br.offsets[idx] + tr.u.buf.offset, tr.u.buf.size, tr.u.buf.data);
    }

    // Staging buffer.
    else if (br.buffer->type == VKL_BUFFER_TYPE_STAGING)
    {
        // The staging buffer must be constantly mapped.
        ASSERT(br.buffer->mmap != NULL);
        ASSERT(br.count == 1);
        vkl_buffer_download(
            br.buffer, br.offsets[0] + tr.u.buf.offset, tr.u.buf.size, tr.u.buf.data);
    }

    // All other (non-mappable) buffers. Require synchronization and copy on command
    // buffer.
    else
    {
        ASSERT(br.count == 1);

        // Take the staging buffer and ensure it is big enough.
        VklBuffer* staging = staging_buffer(context, tr.u.buf.size);

        // Copy from the source buffer to the staging buffer.
        _copy_buffer_to_staging(context, tr.u.buf.regions, tr.u.buf.offset, tr.u.buf.size);

        // Memcpy into the staging buffer.
        vkl_buffer_download(staging, 0, tr.u.buf.size, tr.u.buf.data);
    }
}



static void _process_buffer_copy(VklCanvas* canvas, VklTransfer tr)
{
    ASSERT(canvas != NULL);
    VklGpu* gpu = canvas->gpu;
    ASSERT(gpu != NULL);
    VklContext* context = canvas->gpu->context;
    ASSERT(tr.type == VKL_TRANSFER_BUFFER_COPY);

    VklBufferRegions* src = &tr.u.buf_copy.src;
    VklBufferRegions* dst = &tr.u.buf_copy.dst;
    ASSERT(src->count == dst->count);

    VkDeviceSize size = tr.u.buf_copy.size;
    VkDeviceSize src_offset = tr.u.buf_copy.src_offset;
    VkDeviceSize dst_offset = tr.u.buf_copy.dst_offset;

    // Take transfer cmd buf.
    VklCommands* cmds = &context->transfer_cmd;
    vkl_cmd_reset(cmds, 0);
    vkl_cmd_begin(cmds, 0);

    // Copy buffer command.
    VkBufferCopy* regions = (VkBufferCopy*)calloc(src->count, sizeof(VkBufferCopy));
    for (uint32_t i = 0; i < src->count; i++)
    {
        regions[i].size = size;
        regions[i].srcOffset = src->offsets[i] + src_offset;
        regions[i].dstOffset = dst->offsets[i] + dst_offset;
    }
    vkCmdCopyBuffer(cmds->cmds[0], src->buffer->buffer, dst->buffer->buffer, src->count, regions);

    vkl_cmd_end(cmds, 0);
    FREE(regions);

    // Wait for the render queue to be idle.
    vkl_queue_wait(gpu, VKL_DEFAULT_QUEUE_RENDER);

    // Submit the commands to the transfer queue.
    VklSubmit submit = vkl_submit(gpu);
    vkl_submit_commands(&submit, cmds);
    log_debug("copy %s between 2 buffers", pretty_size(size));
    vkl_submit_send(&submit, 0, NULL, 0);

    // Wait for the transfer queue to be idle.
    vkl_queue_wait(gpu, VKL_DEFAULT_QUEUE_TRANSFER);
}



/*************************************************************************************************/
/*  Canvas transfers processing                                                                  */
/*************************************************************************************************/

void vkl_process_transfers(VklCanvas* canvas)
{
    // This function is to be called at every frame, after the FRAME callbacks (so that FRAME
    // callbacks calling vkl_upload_buffers() have their transfers processed immediately in the
    // same frame), but before queue submit, so that we may get a chance to ask for a command
    // buffer refill before submission (if a transfer requires a refill, e.g. after a vertex buffer
    // count change)
    ASSERT(canvas != NULL);
    VklGpu* gpu = canvas->gpu;
    ASSERT(gpu != NULL);
    VklContext* context = canvas->gpu->context;
    ASSERT(context != NULL);
    VklFifo* fifo = &canvas->transfers;
    // Do nothing if there are no pending transfers.
    if (fifo->is_empty)
        return;

    // Process all pending transfer tasks.
    VklTransfer tr = {0};
    while (true)
    {
        tr = fifo_dequeue(context, fifo, false);
        if (tr.type == VKL_TRANSFER_NONE)
            break;
        fifo->is_processing = true;

        // Process buffer transfers.
        if (tr.type == VKL_TRANSFER_BUFFER_UPLOAD)
            _process_buffer_upload(canvas, tr);
        if (tr.type == VKL_TRANSFER_BUFFER_DOWNLOAD)
            _process_buffer_download(canvas, tr);
        if (tr.type == VKL_TRANSFER_BUFFER_COPY)
            _process_buffer_copy(canvas, tr);

        // Process texture transfers.
        if (tr.type == VKL_TRANSFER_TEXTURE_UPLOAD)
            vkl_texture_upload(
                tr.u.tex.texture, tr.u.tex.offset, tr.u.tex.shape, tr.u.tex.size, tr.u.tex.data);
        if (tr.type == VKL_TRANSFER_TEXTURE_DOWNLOAD)
            vkl_texture_download(
                tr.u.tex.texture, tr.u.tex.offset, tr.u.tex.shape, tr.u.tex.size, tr.u.tex.data);
        if (tr.type == VKL_TRANSFER_TEXTURE_COPY)
            vkl_texture_copy(
                tr.u.tex_copy.src, tr.u.tex_copy.src_offset, tr.u.tex_copy.dst,
                tr.u.tex_copy.dst_offset, tr.u.tex_copy.shape);

        fifo->is_processing = false;
    }
}



/*************************************************************************************************/
/*  Canvas buffer transfers                                                                      */
/*************************************************************************************************/

static void _enqueue_buffers_transfer(
    VklCanvas* canvas, VklDataTransferType type, VklBufferRegions br, //
    VkDeviceSize offset, VkDeviceSize size, void* data)
{
    ASSERT(canvas != NULL);
    ASSERT(canvas->gpu != NULL);
    VklContext* context = canvas->gpu->context;
    ASSERT(context != NULL);
    ASSERT(canvas->transfers.capacity > 0);
    ASSERT(size > 0);
    ASSERT(br.buffer != NULL);
    ASSERT(is_obj_created(&br.buffer->obj));
    ASSERT(data != NULL);

    // Create the transfer object.
    VklTransfer tr = {0};
    tr.type = type;
    tr.u.buf.regions = br;
    tr.u.buf.offset = offset;
    tr.u.buf.size = size;
    tr.u.buf.data = data;

    // HACK: when uploading buffers when the app is not running (for example at initialization)
    // we upload all copies of the VklBufferRegions. This is used when using UNIFORM_MAPPABLE
    // buffers that are not continuously updated in each frame.
    tr.u.buf.update_all_buffers = !canvas->app->is_running;

    fifo_enqueue(context, &canvas->transfers, tr);
}



// WARNING: these functions require that the pointer lives through the next frame (no copy)
void vkl_upload_buffers(
    VklCanvas* canvas, VklBufferRegions br, VkDeviceSize offset, VkDeviceSize size, void* data)
{
    _enqueue_buffers_transfer(canvas, VKL_TRANSFER_BUFFER_UPLOAD, br, offset, size, data);

    if (!canvas->app->is_running)
        vkl_process_transfers(canvas);
}



void vkl_download_buffers(
    VklCanvas* canvas, VklBufferRegions br, VkDeviceSize offset, VkDeviceSize size, void* data)
{
    _enqueue_buffers_transfer(canvas, VKL_TRANSFER_BUFFER_DOWNLOAD, br, offset, size, data);

    if (!canvas->app->is_running)
        vkl_process_transfers(canvas);
}



void vkl_copy_buffers(
    VklCanvas* canvas, VklBufferRegions src, VkDeviceSize src_offset, //
    VklBufferRegions dst, VkDeviceSize dst_offset, VkDeviceSize size)
{
    ASSERT(canvas != NULL);
    ASSERT(canvas->transfers.capacity > 0);
    ASSERT(size > 0);
    ASSERT(src.buffer != NULL);
    ASSERT(dst.buffer != NULL);
    ASSERT(canvas->gpu != NULL);
    VklContext* context = canvas->gpu->context;
    ASSERT(context != NULL);

    // Create the transfer object.
    VklTransfer tr = {0};
    tr.type = VKL_TRANSFER_BUFFER_COPY;
    tr.u.buf_copy.src = src;
    tr.u.buf_copy.dst = dst;
    tr.u.buf_copy.src_offset = src_offset;
    tr.u.buf_copy.dst_offset = dst_offset;
    tr.u.buf_copy.size = size;

    fifo_enqueue(context, &canvas->transfers, tr);

    if (!canvas->app->is_running)
        vkl_process_transfers(canvas);
}



/*************************************************************************************************/
/*  Canvas texture transfers                                                                     */
/*************************************************************************************************/

static void _enqueue_texture_transfer(
    VklCanvas* canvas, VklDataTransferType type, VklTexture* texture, //
    uvec3 offset, uvec3 shape, VkDeviceSize size, void* data)
{
    ASSERT(canvas != NULL);
    ASSERT(canvas->gpu != NULL);
    VklContext* context = canvas->gpu->context;
    ASSERT(context != NULL);
    ASSERT(canvas->transfers.capacity > 0);
    ASSERT(texture != NULL);
    ASSERT(is_obj_created(&texture->obj));
    ASSERT(size > 0);
    ASSERT(data != NULL);

    if (shape[0] == 0)
        shape[0] = texture->image->width;
    if (shape[1] == 0)
        shape[1] = texture->image->height;
    if (shape[2] == 0)
        shape[2] = texture->image->depth;

    // Create the transfer object.
    VklTransfer tr = {0};
    tr.type = type;
    for (uint32_t i = 0; i < 3; i++)
    {
        tr.u.tex.shape[i] = shape[i];
        tr.u.tex.offset[i] = offset[i];
    }
    tr.u.tex.size = size;
    tr.u.tex.data = data;
    tr.u.tex.texture = texture;

    fifo_enqueue(context, &canvas->transfers, tr);
}



void vkl_upload_texture(
    VklCanvas* canvas, VklTexture* texture, uvec3 offset, uvec3 shape, VkDeviceSize size,
    void* data)
{
    _enqueue_texture_transfer(
        canvas, VKL_TRANSFER_TEXTURE_UPLOAD, texture, offset, shape, size, data);

    if (!canvas->app->is_running)
        vkl_process_transfers(canvas);
}



void vkl_download_texture(
    VklCanvas* canvas, VklTexture* texture, uvec3 offset, uvec3 shape, VkDeviceSize size,
    void* data)
{
    _enqueue_texture_transfer(
        canvas, VKL_TRANSFER_TEXTURE_DOWNLOAD, texture, offset, shape, size, data);

    if (!canvas->app->is_running)
        vkl_process_transfers(canvas);
}



void vkl_copy_texture(
    VklCanvas* canvas, VklTexture* src, uvec3 src_offset, VklTexture* dst, uvec3 dst_offset,
    uvec3 shape, VkDeviceSize size)
{
    ASSERT(canvas != NULL);
    ASSERT(canvas->gpu != NULL);
    VklContext* context = canvas->gpu->context;
    ASSERT(context != NULL);
    ASSERT(canvas->transfers.capacity > 0);
    ASSERT(src != NULL);
    ASSERT(is_obj_created(&src->obj));
    ASSERT(dst != NULL);
    ASSERT(is_obj_created(&dst->obj));
    ASSERT(size > 0);

    // Create the transfer object.
    VklTransfer tr = {0};
    tr.type = VKL_TRANSFER_TEXTURE_COPY;
    tr.u.tex_copy.src = src;
    tr.u.tex_copy.dst = dst;
    memcpy(tr.u.tex_copy.src_offset, src_offset, sizeof(uvec3));
    memcpy(tr.u.tex_copy.dst_offset, dst_offset, sizeof(uvec3));
    memcpy(tr.u.tex_copy.shape, shape, sizeof(uvec3));

    fifo_enqueue(context, &canvas->transfers, tr);

    if (!canvas->app->is_running)
        vkl_process_transfers(canvas);
}
