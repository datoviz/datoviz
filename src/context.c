#include "../include/visky/context.h"
#include <stdlib.h>



/*************************************************************************************************/
/*  Macros                                                                                       */
/*************************************************************************************************/

#define TO_KB(x) ((x) / (1024.0))



/*************************************************************************************************/
/*  Context                                                                                      */
/*************************************************************************************************/

static void _context_default_queues(VklGpu* gpu)
{
    vkl_gpu_queue(gpu, VKL_QUEUE_TRANSFER, VKL_DEFAULT_QUEUE_TRANSFER);
    vkl_gpu_queue(gpu, VKL_QUEUE_COMPUTE, VKL_DEFAULT_QUEUE_COMPUTE);
    vkl_gpu_queue(gpu, VKL_QUEUE_RENDER, VKL_DEFAULT_QUEUE_RENDER);
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

    // Staging buffer
    buffer = &context->buffers[VKL_DEFAULT_BUFFER_STAGING];
    vkl_buffer_size(buffer, VKL_DEFAULT_BUFFER_STAGING_SIZE);
    vkl_buffer_usage(buffer, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
    vkl_buffer_memory(
        buffer, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    vkl_buffer_create(buffer);

    VkBufferUsageFlagBits transferable =
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

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



VklContext* vkl_context(VklGpu* gpu)
{
    ASSERT(gpu != NULL);
    ASSERT(gpu->obj.status < VKL_OBJECT_STATUS_CREATED);
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
    _context_default_queues(gpu);

    // Create the GPU after the default queues have been set.
    if (gpu->obj.status < VKL_OBJECT_STATUS_CREATED)
        vkl_gpu_create(gpu, 0);

    // Create the default buffers.
    _context_default_buffers(context);

    context->transfer_cmd = vkl_commands(gpu, VKL_DEFAULT_QUEUE_TRANSFER, 1);

    if (pthread_mutex_init(&context->transfer_fifo.lock, NULL) != 0)
        log_error("mutex creation failed");

    gpu->context = context;

    return context;
}



void vkl_context_transfer_mode(VklContext* context, VklTransferMode mode)
{
    ASSERT(context != NULL);
    context->transfer_mode = mode;
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


    log_trace("context destroy buffers");
    for (uint32_t i = 0; i < context->max_buffers; i++)
    {
        if (context->buffers[i].obj.status == VKL_OBJECT_STATUS_NONE)
            break;
        vkl_buffer_destroy(&context->buffers[i]);
    }
    INSTANCES_DESTROY(context->buffers)
    FREE(context->allocated_sizes);


    log_trace("context destroy sets of images");
    for (uint32_t i = 0; i < context->max_images; i++)
    {
        if (context->images[i].obj.status == VKL_OBJECT_STATUS_NONE)
            break;
        vkl_images_destroy(&context->images[i]);
    }
    INSTANCES_DESTROY(context->images)


    log_trace("context destroy samplers");
    for (uint32_t i = 0; i < context->max_samplers; i++)
    {
        if (context->samplers[i].obj.status == VKL_OBJECT_STATUS_NONE)
            break;
        vkl_sampler_destroy(&context->samplers[i]);
    }
    INSTANCES_DESTROY(context->samplers)


    log_trace("context destroy computes");
    for (uint32_t i = 0; i < context->max_computes; i++)
    {
        if (context->computes[i].obj.status == VKL_OBJECT_STATUS_NONE)
            break;
        vkl_compute_destroy(&context->computes[i]);
    }
    INSTANCES_DESTROY(context->computes)

    pthread_mutex_destroy(&context->transfer_fifo.lock);
}



/*************************************************************************************************/
/*  Buffer allocation                                                                            */
/*************************************************************************************************/

VklBufferRegions vkl_alloc_buffers(
    VklContext* context, uint32_t buffer_idx, uint32_t buffer_count, VkDeviceSize size)
{
    ASSERT(context != NULL);
    ASSERT(context->gpu != NULL);

    VklBufferRegions regions = {0};

    if (buffer_idx >= context->max_buffers ||
        context->buffers[buffer_idx].obj.status < VKL_OBJECT_STATUS_CREATED)
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
            regions.buffer, new_size, VKL_DEFAULT_QUEUE_TRANSFER, context->transfer_cmd);
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
    vkl_images_usage(image, VK_IMAGE_USAGE_STORAGE_BIT);
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
/*  Data transfers                                                                               */
/*************************************************************************************************/

static void fifo_enqueue(VklTransferFifo* fifo, VklTransfer transfer)
{
    ASSERT(fifo != NULL);
    pthread_mutex_lock(&fifo->lock);

    fifo->transfers[fifo->head] = transfer;

    fifo->head++;
    if (fifo->head >= VKL_MAX_TRANSFERS)
        fifo->head -= VKL_MAX_TRANSFERS;

    ASSERT(0 <= fifo->head && fifo->head < VKL_MAX_TRANSFERS);
    pthread_mutex_unlock(&fifo->lock);
}



static VklTransfer fifo_dequeue(VklTransferFifo* fifo)
{
    ASSERT(fifo != NULL);
    pthread_mutex_lock(&fifo->lock);

    // Empty queue.
    if (fifo->head == fifo->tail)
        return (VklTransfer){VKL_TRANSFER_NULL, {{{0}}}};

    ASSERT(0 <= fifo->tail && fifo->tail < VKL_MAX_TRANSFERS);

    VklTransfer tr = fifo->transfers[fifo->tail];

    fifo->tail++;
    if (fifo->tail >= VKL_MAX_TRANSFERS)
        fifo->tail -= VKL_MAX_TRANSFERS;

    ASSERT(0 <= fifo->tail && fifo->tail < VKL_MAX_TRANSFERS);
    pthread_mutex_unlock(&fifo->lock);

    return tr;
}



void vkl_texture_upload_region(
    VklTexture* texture, uvec3 offset, uvec3 shape, VkDeviceSize size, const void* data)
{
    ASSERT(texture != NULL);
    ASSERT(texture->context != NULL);
    VklContext* context = texture->context;
    if (context->transfer_mode == VKL_TRANSFER_MODE_ASYNC)
    {
        log_trace("upload in ASYNC mode");
        // context->transfer_fifo.head
    }
    else
    {
        log_trace("upload in SYNC mode");
        // TODO
    }
}



void vkl_texture_upload(VklTexture* texture, VkDeviceSize size, const void* data)
{
    ASSERT(texture != NULL);
    uvec3 shape = {0};
    shape[0] = texture->image->width;
    shape[1] = texture->image->height;
    shape[2] = texture->image->depth;
    vkl_texture_upload_region(texture, (uvec3){0, 0, 0}, shape, size, data);
}



void vkl_texture_download_region(
    VklTexture* texture, uvec3 offset, uvec3 shape, VkDeviceSize size, void* data)
{
    ASSERT(texture != NULL);
    ASSERT(texture->context != NULL);
    // TODO
}



void vkl_texture_download(VklTexture* texture, VkDeviceSize size, void* data)
{
    ASSERT(texture != NULL);
    uvec3 shape = {0};
    shape[0] = texture->image->width;
    shape[1] = texture->image->height;
    shape[2] = texture->image->depth;
    vkl_texture_download_region(texture, (uvec3){0, 0, 0}, shape, size, data);
}



void vkl_context_transfer(VklContext* context)
{
    ASSERT(context != NULL);
    // TODO
}
