#include "../include/visky/context.h"
#include <stdlib.h>



/*************************************************************************************************/
/*  Macros                                                                                       */
/*************************************************************************************************/

#define TO_KB(x) ((x) / (1024.0))



/*************************************************************************************************/
/*  Thread-safe FIFO queue                                                                       */
/*************************************************************************************************/

VklFifo vkl_fifo(int32_t capacity)
{
    log_trace("creating generic FIFO queue with a capacity of %d items", capacity);
    ASSERT(capacity >= 2);
    VklFifo fifo = {0};
    ASSERT(capacity <= VKL_MAX_FIFO_CAPACITY);
    fifo.capacity = capacity;

    if (pthread_mutex_init(&fifo.lock, NULL) != 0)
        log_error("mutex creation failed");
    if (pthread_cond_init(&fifo.cond, NULL) != 0)
        log_error("cond creation failed");

    return fifo;
}



void vkl_fifo_enqueue(VklFifo* fifo, void* item)
{
    ASSERT(fifo != NULL);
    pthread_mutex_lock(&fifo->lock);

    if ((fifo->head + 1) % fifo->capacity != fifo->tail)
    {
        log_trace("enqueue item, head %d, tail %d", fifo->head, fifo->tail);
        fifo->items[fifo->head] = item;
        fifo->head++;
        if (fifo->head >= fifo->capacity)
            fifo->head -= fifo->capacity;
    }
    else
    {
        log_error("FIFO queue is full, reseting it");
        fifo->head = 0;
        fifo->tail = 0;
    }

    ASSERT(0 <= fifo->head && fifo->head < fifo->capacity);
    pthread_cond_signal(&fifo->cond);
    pthread_mutex_unlock(&fifo->lock);
}



void* vkl_fifo_dequeue(VklFifo* fifo, bool wait)
{
    ASSERT(fifo != NULL);
    pthread_mutex_lock(&fifo->lock);

    // Wait until the queue is not empty.
    if (wait)
    {
        log_trace("waiting for the queue to be non-empty");
        while (fifo->head == fifo->tail)
            pthread_cond_wait(&fifo->cond, &fifo->lock);
    }

    // Empty queue.
    if (fifo->head == fifo->tail)
    {
        log_trace("FIFO queue was empty");
        // Don't forget to unlock the mutex before exiting this function.
        pthread_mutex_unlock(&fifo->lock);
        return NULL;
    }

    ASSERT(0 <= fifo->tail && fifo->tail < fifo->capacity);

    log_trace("dequeue item, head %d, tail %d", fifo->head, fifo->tail);
    void* item = fifo->items[fifo->tail];

    fifo->tail++;
    if (fifo->tail >= fifo->capacity)
        fifo->tail -= fifo->capacity;

    ASSERT(0 <= fifo->tail && fifo->tail < fifo->capacity);
    pthread_mutex_unlock(&fifo->lock);

    return item;
}



void vkl_fifo_reset(VklFifo* fifo)
{
    pthread_mutex_lock(&fifo->lock);
    fifo->head = 0;
    fifo->tail = 0;
    pthread_cond_signal(&fifo->cond);
    pthread_mutex_unlock(&fifo->lock);
}



void vkl_fifo_destroy(VklFifo* fifo)
{
    ASSERT(fifo != NULL);
    pthread_mutex_destroy(&fifo->lock);
    pthread_cond_destroy(&fifo->cond);
}



/*************************************************************************************************/
/*  Context                                                                                      */
/*************************************************************************************************/

static void _context_default_queues(VklGpu* gpu, VklWindow* window)
{
    vkl_gpu_queue(gpu, VKL_QUEUE_TRANSFER, VKL_DEFAULT_QUEUE_TRANSFER);
    vkl_gpu_queue(gpu, VKL_QUEUE_COMPUTE, VKL_DEFAULT_QUEUE_COMPUTE);
    vkl_gpu_queue(gpu, VKL_QUEUE_RENDER, VKL_DEFAULT_QUEUE_RENDER);
    if (window != NULL)
        vkl_gpu_queue(gpu, VKL_QUEUE_PRESENT, VKL_DEFAULT_QUEUE_PRESENT);
}



static void _context_default_buffers(VklContext* context)
{
    // Create a predetermined set of buffers.
    VklBuffer* buffer = NULL;
    for (uint32_t i = 0; i < VKL_DEFAULT_BUFFER_COUNT; i++)
    {
        context->buffers[i] = vkl_buffer(context->gpu);
        buffer = &context->buffers[i];

        // All buffers may be accessed from these queues.
        vkl_buffer_queue_access(buffer, VKL_DEFAULT_QUEUE_TRANSFER);
        vkl_buffer_queue_access(buffer, VKL_DEFAULT_QUEUE_COMPUTE);
        vkl_buffer_queue_access(buffer, VKL_DEFAULT_QUEUE_RENDER);
    }

    VkBufferUsageFlagBits transferable =
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

    // Staging buffer
    buffer = &context->buffers[VKL_DEFAULT_BUFFER_STAGING];
    vkl_buffer_size(buffer, VKL_DEFAULT_BUFFER_STAGING_SIZE);
    vkl_buffer_usage(buffer, transferable);
    vkl_buffer_memory(
        buffer, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    vkl_buffer_create(buffer);

    // Vertex buffer
    buffer = &context->buffers[VKL_DEFAULT_BUFFER_VERTEX];
    vkl_buffer_size(buffer, VKL_DEFAULT_BUFFER_VERTEX_SIZE);
    vkl_buffer_usage(buffer, transferable | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    vkl_buffer_memory(buffer, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    vkl_buffer_create(buffer);

    // Index buffer
    buffer = &context->buffers[VKL_DEFAULT_BUFFER_INDEX];
    vkl_buffer_size(buffer, VKL_DEFAULT_BUFFER_INDEX_SIZE);
    vkl_buffer_usage(buffer, transferable | VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
    vkl_buffer_memory(buffer, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    vkl_buffer_create(buffer);

    // Storage buffer
    buffer = &context->buffers[VKL_DEFAULT_BUFFER_STORAGE];
    vkl_buffer_size(buffer, VKL_DEFAULT_BUFFER_STORAGE_SIZE);
    vkl_buffer_usage(buffer, transferable | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
    vkl_buffer_memory(buffer, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    vkl_buffer_create(buffer);

    // Uniform buffer
    buffer = &context->buffers[VKL_DEFAULT_BUFFER_UNIFORM];
    vkl_buffer_size(buffer, VKL_DEFAULT_BUFFER_UNIFORM_SIZE);
    vkl_buffer_usage(buffer, transferable | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
    vkl_buffer_memory(buffer, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    vkl_buffer_create(buffer);
}



static void _destroy_resources(VklContext* context)
{
    ASSERT(context != NULL);

    log_trace("context destroy buffers");
    for (uint32_t i = 0; i < context->max_buffers; i++)
    {
        if (context->buffers[i].obj.status == VKL_OBJECT_STATUS_NONE)
            break;
        vkl_buffer_destroy(&context->buffers[i]);
    }

    log_trace("context destroy sets of images");
    for (uint32_t i = 0; i < context->max_images; i++)
    {
        if (context->images[i].obj.status == VKL_OBJECT_STATUS_NONE)
            break;
        vkl_images_destroy(&context->images[i]);
    }

    log_trace("context destroy samplers");
    for (uint32_t i = 0; i < context->max_samplers; i++)
    {
        if (context->samplers[i].obj.status == VKL_OBJECT_STATUS_NONE)
            break;
        vkl_sampler_destroy(&context->samplers[i]);
    }

    log_trace("context destroy computes");
    for (uint32_t i = 0; i < context->max_computes; i++)
    {
        if (context->computes[i].obj.status == VKL_OBJECT_STATUS_NONE)
            break;
        vkl_compute_destroy(&context->computes[i]);
    }
}



VklContext* vkl_context(VklGpu* gpu, VklWindow* window)
{
    ASSERT(gpu != NULL);
    ASSERT(!is_obj_created(&gpu->obj));
    log_trace("creating context");

    VklContext* context = calloc(1, sizeof(VklContext));
    context->gpu = gpu;

    // Allocate memory for buffers, textures, and computes.
    INSTANCES_INIT(
        VklBuffer, context, buffers, max_buffers, VKL_MAX_BUFFERS, VKL_OBJECT_TYPE_BUFFER)
    context->allocated_sizes = calloc(context->max_buffers, sizeof(VkDeviceSize));

    INSTANCES_INIT(
        VklTexture, context, textures, max_textures, VKL_MAX_TEXTURES, VKL_OBJECT_TYPE_TEXTURE)

    INSTANCES_INIT(
        VklImages, context, images, max_images, VKL_MAX_TEXTURES, VKL_OBJECT_TYPE_IMAGES)

    INSTANCES_INIT(
        VklSampler, context, samplers, max_samplers, VKL_MAX_TEXTURES, VKL_OBJECT_TYPE_SAMPLER)

    INSTANCES_INIT(
        VklCompute, context, computes, max_computes, VKL_MAX_COMPUTES, VKL_OBJECT_TYPE_COMPUTE)

    // Specify the default queues.
    _context_default_queues(gpu, window);

    // Create the GPU after the default queues have been set.
    if (!is_obj_created(&gpu->obj))
    {
        VkSurfaceKHR surface = VK_NULL_HANDLE;
        if (window != NULL)
            surface = window->surface;
        vkl_gpu_create(gpu, surface);
    }

    // Create the default buffers.
    _context_default_buffers(context);

    context->transfer_cmd = vkl_commands(gpu, VKL_DEFAULT_QUEUE_TRANSFER, 1);
    context->transfer_fifo.queue = vkl_fifo(VKL_MAX_FIFO_CAPACITY);

    gpu->context = context;
    obj_created(&context->obj);

    return context;
}



void vkl_context_reset(VklContext* context)
{
    ASSERT(context != NULL);
    log_trace("reset the context");
    _destroy_resources(context);
    _context_default_buffers(context);
}



void vkl_context_destroy(VklContext* context)
{
    if (context == NULL)
    {
        log_error("skip destruction of null context");
        return;
    }
    log_trace("destroying context");
    ASSERT(context != NULL);
    ASSERT(context->gpu != NULL);

    // Destroy the buffers, images, samplers, textures, computes.
    _destroy_resources(context);

    // Free the allocated memory.
    INSTANCES_DESTROY(context->buffers)
    INSTANCES_DESTROY(context->images)
    INSTANCES_DESTROY(context->samplers)
    INSTANCES_DESTROY(context->computes)
    INSTANCES_DESTROY(context->textures);
    FREE(context->allocated_sizes);

    vkl_fifo_destroy(&context->transfer_fifo.queue);
}



/*************************************************************************************************/
/*  Buffer allocation                                                                            */
/*************************************************************************************************/

VklBufferRegions vkl_alloc_buffers(
    VklContext* context, uint32_t buffer_idx, uint32_t buffer_count, VkDeviceSize size)
{
    ASSERT(context != NULL);
    ASSERT(context->gpu != NULL);
    ASSERT(buffer_count > 0);

    VklBufferRegions regions = {0};

    if (buffer_idx >= context->max_buffers || !is_obj_created(&context->buffers[buffer_idx].obj))
    {
        log_error("invalid buffer #%d", buffer_idx);
        return regions;
    }

    regions.buffer = &context->buffers[buffer_idx];
    regions.count = buffer_count;
    regions.size = size;

    // Need to reallocate?
    if (context->allocated_sizes[buffer_idx] + size * buffer_count > regions.buffer->size)
    {
        VkDeviceSize new_size = regions.buffer->size * 2;
        log_info("reallocating buffer #%d to %.3f KB", buffer_idx, TO_KB(new_size));
        vkl_buffer_resize(
            regions.buffer, new_size, VKL_DEFAULT_QUEUE_TRANSFER, &context->transfer_cmd);
    }

    log_trace("allocating %d buffers with size %.3f KB", buffer_count, TO_KB(size));
    ASSERT(context->allocated_sizes[buffer_idx] + size * buffer_count <= regions.buffer->size);
    for (uint32_t i = 0; i < buffer_count; i++)
    {
        regions.offsets[i] = context->allocated_sizes[buffer_idx] + i * size;
    }
    context->allocated_sizes[buffer_idx] += size * buffer_count;
    ASSERT(regions.offsets[buffer_count - 1] + size == context->allocated_sizes[buffer_idx]);

    return regions;
}



/*************************************************************************************************/
/*  Compute                                                                                      */
/*************************************************************************************************/

VklCompute* vkl_new_compute(VklContext* context, const char* shader_path)
{
    ASSERT(context != NULL);
    ASSERT(shader_path != NULL);

    INSTANCE_NEW(VklCompute, compute, context->computes, context->max_computes);

    *compute = vkl_compute(context->gpu, shader_path);

    return compute;
}



/*************************************************************************************************/
/*  Texture                                                                                      */
/*************************************************************************************************/

static VkImageType image_type_from_dims(uint32_t dims)
{
    switch (dims)
    {
    case 1:
        return VK_IMAGE_TYPE_1D;
        break;
    case 2:
        return VK_IMAGE_TYPE_2D;
        break;
    case 3:
        return VK_IMAGE_TYPE_3D;
        break;

    default:
        break;
    }
    log_error("invalid image dimensions %d", dims);
    return VK_IMAGE_TYPE_2D;
}



VklTexture* vkl_new_texture(VklContext* context, uint32_t dims, uvec3 size, VkFormat format)
{
    ASSERT(context != NULL);

    INSTANCE_NEW(VklTexture, texture, context->textures, context->max_textures);
    INSTANCE_NEW(VklImages, image, context->images, context->max_images);
    INSTANCE_NEW(VklSampler, sampler, context->samplers, context->max_samplers);

    texture->context = context;
    *image = vkl_images(context->gpu, image_type_from_dims(dims), 1);
    *sampler = vkl_sampler(context->gpu);

    texture->image = image;
    texture->sampler = sampler;

    // Create the image.
    vkl_images_format(image, format);
    vkl_images_size(image, size[0], size[1], size[2]);
    vkl_images_tiling(image, VK_IMAGE_TILING_OPTIMAL);
    vkl_images_layout(image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    vkl_images_usage(
        image, //
        VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
            VK_IMAGE_USAGE_TRANSFER_DST_BIT);
    vkl_images_memory(image, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    vkl_images_queue_access(image, VKL_DEFAULT_QUEUE_TRANSFER);
    vkl_images_queue_access(image, VKL_DEFAULT_QUEUE_COMPUTE);
    vkl_images_queue_access(image, VKL_DEFAULT_QUEUE_RENDER);
    vkl_images_create(image);

    // Create the sampler.
    vkl_sampler_min_filter(sampler, VK_FILTER_NEAREST);
    vkl_sampler_mag_filter(sampler, VK_FILTER_NEAREST);
    vkl_sampler_address_mode(sampler, VKL_TEXTURE_AXIS_U, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
    vkl_sampler_address_mode(sampler, VKL_TEXTURE_AXIS_V, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
    vkl_sampler_address_mode(sampler, VKL_TEXTURE_AXIS_V, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
    vkl_sampler_create(sampler);

    obj_created(&texture->obj);

    return texture;
}



void vkl_texture_resize(VklTexture* texture, uvec3 size)
{
    ASSERT(texture != NULL);
    ASSERT(texture->image != NULL);

    vkl_images_resize(texture->image, size[0], size[1], size[2]);
}



void vkl_texture_filter(VklTexture* texture, VklFilterType type, VkFilter filter)
{
    ASSERT(texture != NULL);
    ASSERT(texture->sampler != NULL);

    switch (type)
    {
    case VKL_FILTER_MIN:
        vkl_sampler_min_filter(texture->sampler, filter);
        break;
    case VKL_FILTER_MAX:
        vkl_sampler_mag_filter(texture->sampler, filter);
        break;
    default:
        log_error("invalid filter type %d", type);
        break;
    }
    vkl_sampler_destroy(texture->sampler);
    vkl_sampler_create(texture->sampler);
}



void vkl_texture_address_mode(
    VklTexture* texture, VklTextureAxis axis, VkSamplerAddressMode address_mode)
{
    ASSERT(texture != NULL);
    ASSERT(texture->sampler != NULL);

    vkl_sampler_address_mode(texture->sampler, axis, address_mode);

    vkl_sampler_destroy(texture->sampler);
    vkl_sampler_create(texture->sampler);
}



void vkl_texture_destroy(VklTexture* texture)
{
    ASSERT(texture != NULL);
    vkl_images_destroy(texture->image);
    vkl_sampler_destroy(texture->sampler);

    texture->image = NULL;
    texture->sampler = NULL;
    obj_destroyed(&texture->obj);
}



/*************************************************************************************************/
/*  Data transfers utils                                                                         */
/*************************************************************************************************/

static void process_texture_upload(VklContext* context, VklTransfer tr)
{
    ASSERT(context != NULL);

    VklGpu* gpu = context->gpu;
    ASSERT(gpu != NULL);

    ASSERT(tr.type == VKL_TRANSFER_TEXTURE_UPLOAD);

    // Wait for the transfer queue to be idle.
    vkl_queue_wait(gpu, VKL_DEFAULT_QUEUE_TRANSFER);

    // Take the staging buffer.
    VklBuffer* staging = &context->buffers[VKL_DEFAULT_BUFFER_STAGING];

    // Size of the buffer to transfer.
    VkDeviceSize size = tr.u.tex.size;

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
    VklBuffer* staging = &context->buffers[VKL_DEFAULT_BUFFER_STAGING];

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
    vkl_buffer_download(staging, 0, tr.u.tex.size, tr.u.tex.data);
}



static void process_buffer_upload(VklContext* context, VklTransfer tr)
{
    ASSERT(context != NULL);

    VklGpu* gpu = context->gpu;
    ASSERT(gpu != NULL);

    ASSERT(tr.type == VKL_TRANSFER_BUFFER_UPLOAD);

    // Wait for the transfer queue to be idle.
    vkl_queue_wait(gpu, VKL_DEFAULT_QUEUE_TRANSFER);

    // Take the staging buffer.
    VklBuffer* staging = &context->buffers[VKL_DEFAULT_BUFFER_STAGING];

    // Size of the buffer to transfer.
    VkDeviceSize size = tr.u.buf.size;

    // Transfer from the CPU to the GPU staging buffer.
    vkl_buffer_upload(staging, 0, size, (const void*)tr.u.buf.data);

    // Take transfer cmd buf.
    VklCommands* cmds = &context->transfer_cmd;
    vkl_cmd_reset(cmds, 0);
    vkl_cmd_begin(cmds, 0);

    // Determine the offset in the target buffer.
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
    vkl_cmd_copy_buffer(cmds, 0, staging, 0, tr.u.buf.regions.buffer, offset, size * n_regions);
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

    // Take the staging buffer.
    VklBuffer* staging = &context->buffers[VKL_DEFAULT_BUFFER_STAGING];

    // Take transfer cmd buf.
    VklCommands* cmds = &context->transfer_cmd;
    vkl_cmd_reset(cmds, 0);
    vkl_cmd_begin(cmds, 0);

    // Size of the buffer to transfer.
    VkDeviceSize size = tr.u.buf.size;

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
    default:
        log_error("unknown transfer type %d", tr.type);
        break;
    }
    return 0;
}



/*************************************************************************************************/
/*  Transfer queue                                                                               */
/*************************************************************************************************/

static void fifo_enqueue(VklTransferFifo* fifo, VklTransfer transfer)
{
    ASSERT(fifo != NULL);
    ASSERT(0 <= fifo->queue.head && fifo->queue.head < fifo->queue.capacity);
    fifo->transfers[fifo->queue.head] = transfer;
    vkl_fifo_enqueue(&fifo->queue, &fifo->transfers[fifo->queue.head]);
}



static VklTransfer fifo_dequeue(VklTransferFifo* fifo, bool wait)
{
    ASSERT(fifo != NULL);
    VklTransfer* item = vkl_fifo_dequeue(&fifo->queue, wait);
    if (item == NULL)
        return (VklTransfer){0};
    ASSERT(item != NULL);
    return *item;
}



static VklTransfer enqueue_texture_transfer(
    VklTransferFifo* fifo, VklDataTransferType type, VklTexture* texture, uvec3 offset,
    uvec3 shape, VkDeviceSize size, void* data)
{
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

    fifo_enqueue(fifo, tr);

    return tr;
}



static VklTransfer enqueue_regions_transfer(
    VklTransferFifo* fifo, VklDataTransferType type, VklBufferRegions regions, VkDeviceSize offset,
    VkDeviceSize size, void* data)
{
    // Create the transfer object.
    VklTransfer tr = {0};
    tr.type = type;
    tr.u.buf.regions = regions;
    tr.u.buf.offset = offset;
    tr.u.buf.size = size;
    tr.u.buf.data = data;

    fifo_enqueue(fifo, tr);

    return tr;
}



void vkl_transfer_mode(VklContext* context, VklTransferMode mode)
{
    ASSERT(context != NULL);
    context->transfer_mode = mode;
}



void vkl_transfer_loop(VklContext* context, bool wait)
{
    ASSERT(context != NULL);
    VklTransfer tr = {0};
    int res = 0;
    while (res == 0)
    {
        log_trace("transfer loop awaits for transfer task...");
        // wait until a transfer task is available
        tr = fifo_dequeue(&context->transfer_fifo, wait);
        log_trace("transfer task dequeued, processing it...");
        // process the dequeued task
        res = process_transfer(context, tr);
    }
    log_trace("end transfer loop");
}



void vkl_transfer_stop(VklContext* context)
{
    ASSERT(context != NULL);
    // Enqueue a special object that causes the dequeue loop to end.
    VklTransfer tr = {0};
    tr.type = VKL_TRANSFER_NONE;
    fifo_enqueue(&context->transfer_fifo, tr);
}



/*************************************************************************************************/
/*  Data transfers                                                                               */
/*************************************************************************************************/

void vkl_texture_upload_region(
    VklContext* context, VklTexture* texture, uvec3 offset, uvec3 shape, VkDeviceSize size,
    void* data)
{
    ASSERT(texture != NULL);
    ASSERT(context != NULL);

    VklTransfer tr = enqueue_texture_transfer(
        &context->transfer_fifo, VKL_TRANSFER_TEXTURE_UPLOAD, //
        texture, offset, shape, size, data);

    if (context->transfer_mode == VKL_TRANSFER_MODE_ASYNC)
    {
        log_trace("upload texture in ASYNC mode");
    }
    else
    {
        log_trace("upload texture in SYNC mode");
        process_transfer(context, tr);
    }
}



void vkl_texture_upload(VklContext* context, VklTexture* texture, VkDeviceSize size, void* data)
{
    ASSERT(texture != NULL);
    ASSERT(context != NULL);

    uvec3 shape = {0};
    shape[0] = texture->image->width;
    shape[1] = texture->image->height;
    shape[2] = texture->image->depth;
    vkl_texture_upload_region(context, texture, (uvec3){0, 0, 0}, shape, size, data);
}



void vkl_texture_download_region(
    VklContext* context, VklTexture* texture, uvec3 offset, uvec3 shape, VkDeviceSize size,
    void* data)
{
    ASSERT(texture != NULL);
    ASSERT(context != NULL);

    VklTransfer tr = enqueue_texture_transfer(
        &context->transfer_fifo, VKL_TRANSFER_TEXTURE_DOWNLOAD, //
        texture, offset, shape, size, data);

    if (context->transfer_mode == VKL_TRANSFER_MODE_ASYNC)
    {
        log_trace("download texture in ASYNC mode");
    }
    else
    {
        log_trace("download texture in SYNC mode");
        process_transfer(context, tr);
    }
}



void vkl_texture_download(VklContext* context, VklTexture* texture, VkDeviceSize size, void* data)
{
    ASSERT(texture != NULL);
    ASSERT(context != NULL);

    uvec3 shape = {0};
    shape[0] = texture->image->width;
    shape[1] = texture->image->height;
    shape[2] = texture->image->depth;
    vkl_texture_download_region(context, texture, (uvec3){0, 0, 0}, shape, size, data);
}



void vkl_buffer_regions_upload(
    VklContext* context, VklBufferRegions* regions, VkDeviceSize offset, VkDeviceSize size,
    void* data)
{
    ASSERT(regions != NULL);
    ASSERT(context != NULL);

    VklTransfer tr = enqueue_regions_transfer(
        &context->transfer_fifo, VKL_TRANSFER_BUFFER_UPLOAD, *regions, offset, size, data);

    if (context->transfer_mode == VKL_TRANSFER_MODE_ASYNC)
    {
        log_trace("upload buffer regions in ASYNC mode");
    }
    else
    {
        log_trace("upload buffer regions in SYNC mode");
        process_transfer(context, tr);
    }
}



void vkl_buffer_regions_download(
    VklContext* context, VklBufferRegions* regions, VkDeviceSize offset, VkDeviceSize size,
    void* data)
{
    ASSERT(regions != NULL);
    ASSERT(context != NULL);

    VklTransfer tr = enqueue_regions_transfer(
        &context->transfer_fifo, VKL_TRANSFER_BUFFER_DOWNLOAD, *regions, offset, size, data);

    if (context->transfer_mode == VKL_TRANSFER_MODE_ASYNC)
    {
        log_trace("download buffer regions in ASYNC mode");
    }
    else
    {
        log_trace("download buffer regions in SYNC mode");
        process_transfer(context, tr);
    }
}
