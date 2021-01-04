#include "../include/visky/common.h"
#include "../include/visky/context.h"
#include "../include/visky/vklite.h"
#include "vklite_utils.h"

// included in context.c



/*************************************************************************************************/
/*  Data transfers utils                                                                         */
/*************************************************************************************************/

static VklBuffer* staging_buffer(VklContext* context, VkDeviceSize size)
{
    VklBuffer* staging = vkl_container_get(&context->buffers, VKL_BUFFER_TYPE_STAGING);
    ASSERT(staging != NULL);
    ASSERT(staging->buffer != VK_NULL_HANDLE);
    // Resize the staging buffer is needed.
    // TODO: keep staging buffer fixed and copy parts of the data to staging buffer in several
    // steps.
    if (staging->size < size)
    {
        VkDeviceSize new_size = next_pow2(size);
        log_info(
            "reallocating staging buffer to %s", VKL_BUFFER_TYPE_STAGING, pretty_size(new_size));
        vkl_buffer_resize(staging, new_size, VKL_DEFAULT_QUEUE_TRANSFER, &context->transfer_cmd);
    }
    ASSERT(staging->size >= size);
    return staging;
}



static void process_texture_upload(VklContext* context, VklTransfer tr)
{
    ASSERT(context != NULL);

    VklGpu* gpu = context->gpu;
    ASSERT(gpu != NULL);

    ASSERT(tr.type == VKL_TRANSFER_TEXTURE_UPLOAD);

    // Wait for the transfer queue to be idle.
    vkl_queue_wait(gpu, VKL_DEFAULT_QUEUE_TRANSFER);

    // Size of the buffer to transfer.
    VkDeviceSize size = tr.u.tex.size;

    // Take the staging buffer.
    VklBuffer* staging = staging_buffer(context, size);

    // Transfer from the CPU to the GPU staging buffer.
    vkl_buffer_upload(staging, 0, size, (const void*)tr.u.tex.data);

    // Take transfer cmd buf.
    VklCommands* cmds = &context->transfer_cmd;
    vkl_cmd_reset(cmds, 0);
    vkl_cmd_begin(cmds, 0);

    // Image transition.
    VklBarrier barrier = vkl_barrier(gpu);
    vkl_barrier_stages(&barrier, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
    ASSERT(tr.u.tex.texture != NULL);
    ASSERT(tr.u.tex.texture->image != NULL);
    vkl_barrier_images(&barrier, tr.u.tex.texture->image);
    vkl_barrier_images_layout(
        &barrier, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    vkl_barrier_images_access(&barrier, 0, VK_ACCESS_TRANSFER_WRITE_BIT);
    vkl_cmd_barrier(cmds, 0, &barrier);

    // Copy to staging buffer
    vkl_cmd_copy_buffer_to_image(cmds, 0, staging, tr.u.tex.texture->image);

    // Image transition.
    vkl_barrier_images_layout(
        &barrier, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, tr.u.tex.texture->image->layout);
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
    vkl_queue_wait(gpu, VKL_DEFAULT_QUEUE_TRANSFER);
}



static void process_texture_download(VklContext* context, VklTransfer tr)
{
    ASSERT(context != NULL);

    VklGpu* gpu = context->gpu;
    ASSERT(gpu != NULL);

    ASSERT(tr.type == VKL_TRANSFER_TEXTURE_DOWNLOAD);

    // Take the staging buffer.
    VkDeviceSize size = tr.u.tex.size;
    VklBuffer* staging = staging_buffer(context, size);

    // Take transfer cmd buf.
    VklCommands* cmds = &context->transfer_cmd;
    vkl_cmd_reset(cmds, 0);
    vkl_cmd_begin(cmds, 0);

    // Image transition.
    VklBarrier barrier = vkl_barrier(gpu);
    vkl_barrier_stages(&barrier, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
    ASSERT(tr.u.tex.texture != NULL);
    ASSERT(tr.u.tex.texture->image != NULL);
    vkl_barrier_images(&barrier, tr.u.tex.texture->image);
    vkl_barrier_images_layout(
        &barrier, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
    vkl_barrier_images_access(&barrier, 0, VK_ACCESS_TRANSFER_READ_BIT);
    vkl_cmd_barrier(cmds, 0, &barrier);

    // Copy to staging buffer
    vkl_cmd_copy_image_to_buffer(cmds, 0, tr.u.tex.texture->image, staging);

    // Image transition.
    vkl_barrier_images_layout(
        &barrier, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, tr.u.tex.texture->image->layout);
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
    vkl_queue_wait(gpu, VKL_DEFAULT_QUEUE_TRANSFER);

    // Transfer from the CPU to the GPU staging buffer.
    vkl_buffer_download(staging, 0, size, tr.u.tex.data);
}



static void process_buffer_upload(VklContext* context, VklTransfer tr)
{
    ASSERT(context != NULL);

    VklGpu* gpu = context->gpu;
    ASSERT(gpu != NULL);

    ASSERT(tr.type == VKL_TRANSFER_BUFFER_UPLOAD);

    // Wait for the transfer queue to be idle.
    vkl_queue_wait(gpu, VKL_DEFAULT_QUEUE_TRANSFER);

    // Size of the buffer to transfer.
    VkDeviceSize region_size = tr.u.buf.size;
    ASSERT(region_size > 0);

    VkDeviceSize alsize = tr.u.buf.regions.aligned_size;
    if (alsize == 0)
        alsize = region_size;
    ASSERT(alsize > 0);

    uint32_t n = tr.u.buf.regions.count;
    ASSERT(n > 0);

    // Copy the data as many times as there are buffer regions, and make sure the array is
    // aligned if using a UNIFORM buffer.
    VklPointer pointer = aligned_repeat(region_size, tr.u.buf.data, n, tr.u.buf.regions.alignment);
    // Transfer from the CPU to the GPU staging buffer.
    VkDeviceSize total_size = alsize * n;

    // Take the staging buffer.
    VklBuffer* staging = staging_buffer(context, total_size);

    vkl_buffer_upload(staging, 0, total_size, pointer.pointer);
    ALIGNED_FREE(pointer)

    // Take transfer cmd buf.
    VklCommands* cmds = &context->transfer_cmd;
    vkl_cmd_reset(cmds, 0);
    vkl_cmd_begin(cmds, 0);

    // Determine the offset in the target buffer.
    VkDeviceSize init_offset = tr.u.buf.regions.offsets[0];
    VkDeviceSize sub_offset = tr.u.buf.offset;
    ASSERT(tr.u.buf.regions.buffer != VK_NULL_HANDLE);
    VkBufferCopy* regions = calloc(n, sizeof(VkBufferCopy));
    for (uint32_t i = 0; i < n; i++)
    {
        regions[i].size = region_size;
        regions[i].srcOffset = sub_offset + i * alsize;
        regions[i].dstOffset = init_offset + sub_offset + i * alsize;
    }
    vkCmdCopyBuffer(
        cmds->cmds[0], staging->buffer, tr.u.buf.regions.buffer->buffer, //
        tr.u.buf.regions.count, regions);
    FREE(regions);

    vkl_cmd_end(cmds, 0);

    // Wait for the render queue to be idle.
    vkl_queue_wait(gpu, VKL_DEFAULT_QUEUE_RENDER);

    // Submit the commands to the transfer queue.
    VklSubmit submit = vkl_submit(gpu);
    vkl_submit_commands(&submit, cmds);
    vkl_submit_send(&submit, 0, NULL, 0);

    // Wait for the transfer queue to be idle.
    vkl_queue_wait(gpu, VKL_DEFAULT_QUEUE_TRANSFER);
}



static void process_buffer_download(VklContext* context, VklTransfer tr)
{
    ASSERT(context != NULL);

    VklGpu* gpu = context->gpu;
    ASSERT(gpu != NULL);

    ASSERT(tr.type == VKL_TRANSFER_BUFFER_DOWNLOAD);

    // Take transfer cmd buf.
    VklCommands* cmds = &context->transfer_cmd;
    vkl_cmd_reset(cmds, 0);
    vkl_cmd_begin(cmds, 0);

    // Size of the buffer to transfer.
    VkDeviceSize size = tr.u.buf.size;

    // Take the staging buffer.
    VklBuffer* staging = staging_buffer(context, size);

    // Determine the offset in the source buffer.
    // Should be consecutive offsets.
    VkDeviceSize offset = tr.u.buf.regions.offsets[0];
    uint32_t n_regions = tr.u.buf.regions.count;
    for (uint32_t i = 1; i < n_regions; i++)
    {
        ASSERT(tr.u.buf.regions.offsets[i] == offset + i * size);
    }
    // Take into account the transfer offset.
    offset += tr.u.buf.offset;

    // Copy to staging buffer
    ASSERT(tr.u.buf.regions.buffer != 0);
    vkl_cmd_copy_buffer(cmds, 0, tr.u.buf.regions.buffer, offset, staging, 0, size * n_regions);
    vkl_cmd_end(cmds, 0);

    // Wait for the render queue to be idle.
    vkl_queue_wait(gpu, VKL_DEFAULT_QUEUE_RENDER);

    // Submit the commands to the transfer queue.
    VklSubmit submit = vkl_submit(gpu);
    vkl_submit_commands(&submit, cmds);
    vkl_submit_send(&submit, 0, NULL, 0);

    // Wait for the transfer queue to be idle.
    vkl_queue_wait(gpu, VKL_DEFAULT_QUEUE_TRANSFER);

    // Transfer from the CPU to the GPU staging buffer.
    vkl_buffer_download(staging, 0, size, tr.u.buf.data);
}



static void process_buffer_copy(VklContext* context, VklTransfer tr)
{
    ASSERT(context != NULL);

    VklGpu* gpu = context->gpu;
    ASSERT(gpu != NULL);

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
    vkl_submit_send(&submit, 0, NULL, 0);

    // Wait for the transfer queue to be idle.
    vkl_queue_wait(gpu, VKL_DEFAULT_QUEUE_TRANSFER);
}



static void process_texture_copy(VklContext* context, VklTransfer tr)
{
    ASSERT(context != NULL);

    VklGpu* gpu = context->gpu;
    ASSERT(gpu != NULL);

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
    vkl_submit_send(&submit, 0, NULL, 0);

    // Wait for the transfer queue to be idle.
    vkl_queue_wait(gpu, VKL_DEFAULT_QUEUE_TRANSFER);
}



static int process_transfer(VklContext* context, VklTransfer tr)
{
    ASSERT(context != NULL);
    switch (tr.type)
    {
    case VKL_TRANSFER_NONE:
        return 1;
        break;
    case VKL_TRANSFER_TEXTURE_UPLOAD:
        process_texture_upload(context, tr);
        break;
    case VKL_TRANSFER_TEXTURE_DOWNLOAD:
        process_texture_download(context, tr);
        break;
    case VKL_TRANSFER_BUFFER_UPLOAD:
        process_buffer_upload(context, tr);
        break;
    case VKL_TRANSFER_BUFFER_DOWNLOAD:
        process_buffer_download(context, tr);
        break;
    case VKL_TRANSFER_BUFFER_COPY:
        process_buffer_copy(context, tr);
        break;
    case VKL_TRANSFER_TEXTURE_COPY:
        process_texture_copy(context, tr);
        break;
    default:
        log_error("unknown transfer type %d", tr.type);
        break;
    }
    return 0;
}



/*************************************************************************************************/
/*  Transfer utils                                                                               */
/*************************************************************************************************/

static void fifo_enqueue(VklContext* context, VklTransfer transfer)
{
    ASSERT(context != NULL);
    VklFifo* fifo = &context->fifo;
    ASSERT(0 <= fifo->head && fifo->head < fifo->capacity);
    context->transfers[fifo->head] = transfer;
    vkl_fifo_enqueue(fifo, &context->transfers[fifo->head]);
}



static VklTransfer fifo_dequeue(VklContext* context, bool wait)
{
    ASSERT(context != NULL);
    VklFifo* fifo = &context->fifo;
    VklTransfer* item = vkl_fifo_dequeue(fifo, wait);
    if (item == NULL)
        return (VklTransfer){0};
    ASSERT(item != NULL);
    return *item;
}



static VklTransfer enqueue_texture_transfer(
    VklContext* context, VklDataTransferType type, VklTexture* texture, uvec3 offset, uvec3 shape,
    VkDeviceSize size, void* data)
{
    ASSERT(context != NULL);
    ASSERT(size > 0);
    ASSERT(data != NULL);

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

    if (context->transfer_mode == VKL_TRANSFER_MODE_SYNC)
        process_transfer(context, tr);
    else
        fifo_enqueue(context, tr);

    return tr;
}



static VklTransfer enqueue_regions_transfer(
    VklContext* context, VklDataTransferType type, VklBufferRegions regions, VkDeviceSize offset,
    VkDeviceSize size, void* data)
{
    ASSERT(context != NULL);
    ASSERT(size > 0);
    ASSERT(data != NULL);

    // Create the transfer object.
    VklTransfer tr = {0};
    tr.type = type;
    tr.u.buf.regions = regions;
    tr.u.buf.offset = offset;
    tr.u.buf.size = size;
    tr.u.buf.data = data;

    if (context->transfer_mode == VKL_TRANSFER_MODE_SYNC)
        process_transfer(context, tr);
    else
        fifo_enqueue(context, tr);

    return tr;
}
