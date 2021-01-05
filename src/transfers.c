#include "../include/visky/transfers.h"
#include "../include/visky/canvas.h"
#include "../include/visky/context.h"
#include "../include/visky/fifo.h"



/*************************************************************************************************/
/*  Utils                                                                                        */
/*************************************************************************************************/

static void fifo_enqueue(VklContext* context, VklFifo* fifo, VklTransfer transfer)
{
    ASSERT(context != NULL);
    ASSERT(0 <= fifo->head && fifo->head < fifo->capacity);
    VklTransfer* tr = calloc(1, sizeof(VklTransfer));
    *tr = transfer;
    vkl_fifo_enqueue(fifo, tr);
}



static VklTransfer fifo_dequeue(VklContext* context, VklFifo* fifo, bool wait)
{
    ASSERT(context != NULL);
    VklTransfer* item = vkl_fifo_dequeue(fifo, wait);
    if (item == NULL)
        return (VklTransfer){0};
    ASSERT(item != NULL);
    VklTransfer out = {0};
    out = *item;
    FREE(item);
    return out;
}



static VklBuffer* staging_buffer(VklContext* context, VkDeviceSize size)
{
    VklBuffer* staging = vkl_container_get(&context->buffers, VKL_BUFFER_TYPE_STAGING);
    ASSERT(staging != NULL);
    ASSERT(staging->buffer != VK_NULL_HANDLE);

    // Make sure the staging buffer is idle before using it.
    // TODO: optimize this and avoid hard synchronization here before copying data into
    // the staging buffer.
    vkl_queue_wait(context->gpu, VKL_DEFAULT_QUEUE_TRANSFER);

    // Resize the staging buffer is needed.
    // TODO: keep staging buffer fixed and copy parts of the data to staging buffer in several
    // steps?
    if (staging->size < size)
    {

        VkDeviceSize new_size = next_pow2(size);
        log_info(
            "reallocating staging buffer to %s", VKL_BUFFER_TYPE_STAGING, pretty_size(new_size));
        vkl_buffer_resize(staging, new_size, &context->transfer_cmd);
    }
    ASSERT(staging->size >= size);
    return staging;
}



/*************************************************************************************************/
/*  Buffer upload                                                                                */
/*************************************************************************************************/

static void _copy_buffer_from_staging(
    VklContext* context, VklBufferRegions br, VkDeviceSize offset, VkDeviceSize size)
{
    ASSERT(context != NULL);

    VklGpu* gpu = context->gpu;
    ASSERT(gpu != NULL);

    VklBuffer* staging = staging_buffer(context, size);
    ASSERT(staging != NULL);

    // Take transfer cmd buf.
    VklCommands* cmds = &context->transfer_cmd;
    vkl_cmd_reset(cmds, 0);
    vkl_cmd_begin(cmds, 0);

    VkBufferCopy region = {0};
    region.size = size;
    region.srcOffset = 0;
    region.dstOffset = br.offsets[0] + offset;
    vkCmdCopyBuffer(cmds->cmds[0], staging->buffer, br.buffer->buffer, br.count, &region);
    vkl_cmd_end(cmds, 0);

    // Wait for the render queue to be idle.
    // TODO: less brutal synchronization with semaphores. Here we stop all
    // rendering so that we're sure that the buffer we're going to write to is not
    // being used by the GPU.
    vkl_queue_wait(gpu, VKL_DEFAULT_QUEUE_RENDER);

    // Submit the commands to the transfer queue.
    VklSubmit submit = vkl_submit(gpu);
    vkl_submit_commands(&submit, cmds);
    log_debug("copy %s from staging buffer", pretty_size(size));
    vkl_submit_send(&submit, 0, NULL, 0);

    // Wait for the transfer queue to be idle.
    // TODO: less brutal synchronization with semaphores. Here we wait for the
    // transfer to be complete before we send new rendering commands.
    vkl_queue_wait(gpu, VKL_DEFAULT_QUEUE_TRANSFER);
}



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
        vkl_buffer_upload(
            br.buffer, br.offsets[idx] + tr.u.buf.offset, tr.u.buf.size, tr.u.buf.data);
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



/*************************************************************************************************/
/*  Buffer download                                                                              */
/*************************************************************************************************/

static void _copy_buffer_to_staging(
    VklContext* context, VklBufferRegions br, VkDeviceSize offset, VkDeviceSize size)
{
    ASSERT(context != NULL);

    VklGpu* gpu = context->gpu;
    ASSERT(gpu != NULL);

    VklBuffer* staging = staging_buffer(context, size);
    ASSERT(staging != NULL);

    // Take transfer cmd buf.
    VklCommands* cmds = &context->transfer_cmd;
    vkl_cmd_reset(cmds, 0);
    vkl_cmd_begin(cmds, 0);

    // Determine the offset in the source buffer.
    // Should be consecutive offsets.
    VkDeviceSize vk_offset = br.offsets[0];
    uint32_t n_regions = br.count;
    for (uint32_t i = 1; i < n_regions; i++)
    {
        ASSERT(br.offsets[i] == vk_offset + i * size);
    }
    // Take into account the transfer offset.
    vk_offset += offset;

    // Copy to staging buffer
    ASSERT(br.buffer != 0);
    vkl_cmd_copy_buffer(cmds, 0, br.buffer, vk_offset, staging, 0, size * n_regions);
    vkl_cmd_end(cmds, 0);

    // Wait for the compute queue to be idle, as we assume the buffer to be copied from may
    // be modified by compute shaders.
    // TODO: more efficient synchronization (semaphores?)
    vkl_queue_wait(gpu, VKL_DEFAULT_QUEUE_COMPUTE);

    // Submit the commands to the transfer queue.
    VklSubmit submit = vkl_submit(gpu);
    vkl_submit_commands(&submit, cmds);
    log_debug("copy %s to staging buffer", pretty_size(size));
    vkl_submit_send(&submit, 0, NULL, 0);

    // Wait for the transfer queue to be idle.
    // TODO: less brutal synchronization with semaphores. Here we wait for the
    // transfer to be complete before we send new rendering commands.
    vkl_queue_wait(gpu, VKL_DEFAULT_QUEUE_TRANSFER);
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



/*************************************************************************************************/
/*  Buffer copy                                                                                  */
/*************************************************************************************************/

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
/*  Texture upload                                                                               */
/*************************************************************************************************/

static void _copy_texture_from_staging(
    VklContext* context, VklTexture* texture, uvec3 offset, uvec3 shape, VkDeviceSize size)
{
    ASSERT(context != NULL);

    VklGpu* gpu = context->gpu;
    ASSERT(gpu != NULL);

    VklBuffer* staging = staging_buffer(context, size);
    ASSERT(staging != NULL);

    // Take transfer cmd buf.
    VklCommands* cmds = &context->transfer_cmd;
    vkl_cmd_reset(cmds, 0);
    vkl_cmd_begin(cmds, 0);

    // Image transition.
    VklBarrier barrier = vkl_barrier(gpu);
    vkl_barrier_stages(&barrier, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
    ASSERT(texture != NULL);
    ASSERT(texture->image != NULL);
    vkl_barrier_images(&barrier, texture->image);
    vkl_barrier_images_layout(
        &barrier, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    vkl_barrier_images_access(&barrier, 0, VK_ACCESS_TRANSFER_WRITE_BIT);
    vkl_cmd_barrier(cmds, 0, &barrier);

    // Copy to staging buffer
    vkl_cmd_copy_buffer_to_image(cmds, 0, staging, texture->image);

    // Image transition.
    vkl_barrier_images_layout(
        &barrier, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, texture->image->layout);
    vkl_barrier_images_access(&barrier, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_MEMORY_READ_BIT);
    vkl_cmd_barrier(cmds, 0, &barrier);

    vkl_cmd_end(cmds, 0);

    // Wait for the render queue to be idle.
    vkl_queue_wait(gpu, VKL_DEFAULT_QUEUE_RENDER);

    // Submit the commands to the transfer queue.
    VklSubmit submit = vkl_submit(gpu);
    vkl_submit_commands(&submit, cmds);
    vkl_submit_send(&submit, 0, NULL, 0);

    // Wait for the transfer queue to be idle.
    // TODO: less brutal synchronization with semaphores. Here we wait for the
    // transfer to be complete before we send new rendering commands.
    vkl_queue_wait(gpu, VKL_DEFAULT_QUEUE_TRANSFER);
}



static void _process_texture_upload(VklCanvas* canvas, VklTransfer tr)
{
    ASSERT(canvas != NULL);
    VklGpu* gpu = canvas->gpu;
    ASSERT(gpu != NULL);
    VklContext* context = canvas->gpu->context;
    ASSERT(tr.type == VKL_TRANSFER_TEXTURE_UPLOAD);

    // Size of the buffer to transfer.
    VkDeviceSize size = tr.u.tex.size;

    // Take the staging buffer.
    VklBuffer* staging = staging_buffer(context, size);

    // Memcpy into the staging buffer.
    vkl_buffer_upload(staging, 0, tr.u.tex.size, tr.u.tex.data);

    // Copy from the staging buffer to the texture.
    _copy_texture_from_staging(
        context, tr.u.tex.texture, tr.u.tex.offset, tr.u.tex.shape, tr.u.tex.size);
}



/*************************************************************************************************/
/*  Texture download                                                                             */
/*************************************************************************************************/

static void _copy_texture_to_staging(
    VklContext* context, VklTexture* texture, uvec3 offset, uvec3 shape, VkDeviceSize size)
{
    ASSERT(context != NULL);

    VklGpu* gpu = context->gpu;
    ASSERT(gpu != NULL);

    VklBuffer* staging = staging_buffer(context, size);
    ASSERT(staging != NULL);

    // Take transfer cmd buf.
    VklCommands* cmds = &context->transfer_cmd;
    vkl_cmd_reset(cmds, 0);
    vkl_cmd_begin(cmds, 0);

    // Image transition.
    VklBarrier barrier = vkl_barrier(gpu);
    vkl_barrier_stages(&barrier, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
    ASSERT(texture != NULL);
    ASSERT(texture->image != NULL);
    vkl_barrier_images(&barrier, texture->image);
    vkl_barrier_images_layout(
        &barrier, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
    vkl_barrier_images_access(&barrier, 0, VK_ACCESS_TRANSFER_READ_BIT);
    vkl_cmd_barrier(cmds, 0, &barrier);

    // Copy to staging buffer
    vkl_cmd_copy_image_to_buffer(cmds, 0, texture->image, staging);

    // Image transition.
    vkl_barrier_images_layout(
        &barrier, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, texture->image->layout);
    vkl_barrier_images_access(&barrier, VK_ACCESS_TRANSFER_READ_BIT, VK_ACCESS_MEMORY_READ_BIT);
    vkl_cmd_barrier(cmds, 0, &barrier);

    vkl_cmd_end(cmds, 0);

    // Wait for the render queue to be idle.
    vkl_queue_wait(gpu, VKL_DEFAULT_QUEUE_RENDER);

    // Submit the commands to the transfer queue.
    VklSubmit submit = vkl_submit(gpu);
    vkl_submit_commands(&submit, cmds);
    vkl_submit_send(&submit, 0, NULL, 0);

    // Wait for the transfer queue to be idle.
    // TODO: less brutal synchronization with semaphores. Here we wait for the
    // transfer to be complete before we send new rendering commands.
    vkl_queue_wait(gpu, VKL_DEFAULT_QUEUE_TRANSFER);
}



static void _process_texture_download(VklCanvas* canvas, VklTransfer tr)
{
    ASSERT(canvas != NULL);
    VklGpu* gpu = canvas->gpu;
    ASSERT(gpu != NULL);
    VklContext* context = canvas->gpu->context;
    ASSERT(tr.type == VKL_TRANSFER_TEXTURE_DOWNLOAD);

    // Size of the buffer to transfer.
    VkDeviceSize size = tr.u.tex.size;

    // Take the staging buffer.
    VklBuffer* staging = staging_buffer(context, size);

    // Copy from the staging buffer to the texture.
    _copy_texture_to_staging(
        context, tr.u.tex.texture, tr.u.tex.offset, tr.u.tex.shape, tr.u.tex.size);

    // Memcpy into the staging buffer.
    vkl_buffer_download(staging, 0, tr.u.tex.size, tr.u.tex.data);
}



/*************************************************************************************************/
/*  Texture copy                                                                                 */
/*************************************************************************************************/

static void _process_texture_copy(VklCanvas* canvas, VklTransfer tr)
{
    ASSERT(canvas != NULL);
    VklGpu* gpu = canvas->gpu;
    ASSERT(gpu != NULL);
    VklContext* context = canvas->gpu->context;
    ASSERT(tr.type == VKL_TRANSFER_TEXTURE_COPY);

    VklTexture* src = tr.u.tex_copy.src;
    VklTexture* dst = tr.u.tex_copy.dst;

    // Take transfer cmd buf.
    VklCommands* cmds = &context->transfer_cmd;
    vkl_cmd_reset(cmds, 0);
    vkl_cmd_begin(cmds, 0);

    VklBarrier src_barrier = vkl_barrier(gpu);
    vkl_barrier_stages(
        &src_barrier, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
    vkl_barrier_images(&src_barrier, src->image);

    VklBarrier dst_barrier = vkl_barrier(gpu);
    vkl_barrier_stages(
        &dst_barrier, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
    vkl_barrier_images(&dst_barrier, dst->image);

    // Source image transition.
    if (src->image->layout != VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL)
    {
        vkl_barrier_images_layout(
            &src_barrier, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
        vkl_cmd_barrier(cmds, 0, &src_barrier);
    }

    // Destination image transition.
    {
        log_trace("destination image transition");
        vkl_barrier_images_layout(
            &dst_barrier, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        vkl_cmd_barrier(cmds, 0, &dst_barrier);
    }

    // Copy texture command.
    VkImageCopy copy = {0};
    copy.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    copy.srcSubresource.layerCount = 1;
    copy.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    copy.dstSubresource.layerCount = 1;
    copy.extent.width = tr.u.tex_copy.shape[0];
    copy.extent.height = tr.u.tex_copy.shape[1];
    copy.extent.depth = tr.u.tex_copy.shape[2];
    copy.srcOffset.x = (int32_t)tr.u.tex_copy.src_offset[0];
    copy.srcOffset.y = (int32_t)tr.u.tex_copy.src_offset[1];
    copy.srcOffset.z = (int32_t)tr.u.tex_copy.src_offset[2];
    copy.dstOffset.x = (int32_t)tr.u.tex_copy.dst_offset[0];
    copy.dstOffset.y = (int32_t)tr.u.tex_copy.dst_offset[1];
    copy.dstOffset.z = (int32_t)tr.u.tex_copy.dst_offset[2];
    vkCmdCopyImage(
        cmds->cmds[0],                                               //
        src->image->images[0], VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, //
        dst->image->images[0], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, //
        1, &copy);

    // Source image transition.
    if (src->image->layout != VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL)
    {
        vkl_barrier_images_layout(
            &src_barrier, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, src->image->layout);
        vkl_cmd_barrier(cmds, 0, &src_barrier);
    }

    // Destination image transition.
    if (dst->image->layout != VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
    {
        log_trace("destination image transition back");
        vkl_barrier_images_layout(
            &dst_barrier, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, dst->image->layout);
        vkl_cmd_barrier(cmds, 0, &dst_barrier);
    }

    vkl_cmd_end(cmds, 0);

    // Wait for the render queue to be idle.
    vkl_queue_wait(gpu, VKL_DEFAULT_QUEUE_RENDER);

    // Submit the commands to the transfer queue.
    VklSubmit submit = vkl_submit(gpu);
    vkl_submit_commands(&submit, cmds);
    log_debug(
        "copy %dx%dx%d between 2 textures", tr.u.tex_copy.shape[0], tr.u.tex_copy.shape[1],
        tr.u.tex_copy.shape[2]);
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
            _process_texture_upload(canvas, tr);
        if (tr.type == VKL_TRANSFER_TEXTURE_DOWNLOAD)
            _process_texture_download(canvas, tr);
        if (tr.type == VKL_TRANSFER_TEXTURE_COPY)
            _process_texture_copy(canvas, tr);

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
    VklContext* context = canvas->gpu->context;
    ASSERT(canvas->gpu != NULL);
    ASSERT(context != NULL);
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

    fifo_enqueue(context, &canvas->transfers, tr);
}



void vkl_upload_buffers(
    VklCanvas* canvas, VklBufferRegions br, VkDeviceSize offset, VkDeviceSize size, void* data)
{
    _enqueue_buffers_transfer(canvas, VKL_TRANSFER_BUFFER_UPLOAD, br, offset, size, data);
}



void vkl_download_buffers(
    VklCanvas* canvas, VklBufferRegions br, VkDeviceSize offset, VkDeviceSize size, void* data)
{
    _enqueue_buffers_transfer(canvas, VKL_TRANSFER_BUFFER_DOWNLOAD, br, offset, size, data);
}



void vkl_copy_buffers(
    VklCanvas* canvas, VklBufferRegions src, VkDeviceSize src_offset, //
    VklBufferRegions dst, VkDeviceSize dst_offset, VkDeviceSize size)
{
    ASSERT(canvas != NULL);
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
}



void vkl_download_texture(
    VklCanvas* canvas, VklTexture* texture, uvec3 offset, uvec3 shape, VkDeviceSize size,
    void* data)
{
    _enqueue_texture_transfer(
        canvas, VKL_TRANSFER_TEXTURE_DOWNLOAD, texture, offset, shape, size, data);
}



void vkl_copy_texture(
    VklCanvas* canvas, VklTexture* src, uvec3 src_offset, VklTexture* dst, uvec3 dst_offset,
    uvec3 shape, VkDeviceSize size)
{
    ASSERT(canvas != NULL);
    ASSERT(canvas->gpu != NULL);
    VklContext* context = canvas->gpu->context;
    ASSERT(context != NULL);
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
}
