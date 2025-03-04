/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Holds all GPU data resources (buffers, images, dats, texs)                                   */
/*************************************************************************************************/

#ifndef DVZ_HEADER_RESOURCES_UTILS
#define DVZ_HEADER_RESOURCES_UTILS

#include "_enums.h"
#include "alloc.h"
#include "context.h"
#include "datalloc.h"
#include "host.h"
#include "resources.h"
#include "vklite.h"



/*************************************************************************************************/
/*  Allocs macros                                                                                */
/*************************************************************************************************/

#define CHECK_BUFFER_TYPE                                                                         \
    ASSERT((uint32_t)type >= 1);                                                                  \
    ASSERT((uint32_t)type <= DVZ_BUFFER_TYPE_COUNT);                                              \
    if (type == DVZ_BUFFER_TYPE_STAGING)                                                          \
        mappable = true;



/*************************************************************************************************/
/*  Default buffers                                                                              */
/*************************************************************************************************/

static void _default_queues(DvzGpu* gpu, bool has_present_queue)
{
    dvz_gpu_queue(gpu, DVZ_DEFAULT_QUEUE_TRANSFER, DVZ_QUEUE_TRANSFER);
    dvz_gpu_queue(gpu, DVZ_DEFAULT_QUEUE_COMPUTE, DVZ_QUEUE_COMPUTE);
    dvz_gpu_queue(gpu, DVZ_DEFAULT_QUEUE_RENDER, DVZ_QUEUE_RENDER);
    if (has_present_queue)
        dvz_gpu_queue(gpu, DVZ_DEFAULT_QUEUE_PRESENT, DVZ_QUEUE_PRESENT);
}



static inline bool _is_buffer_mappable(DvzBuffer* buffer)
{
    ANN(buffer);
    return ((buffer->memory & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) != 0);
}



static inline bool _is_buffer_intended_mappable(DvzBuffer* buffer)
{
    ANN(buffer);
    return buffer->mappable_intended;
}



static inline VkBufferUsageFlags _find_buffer_usage(DvzBufferType type)
{
    ASSERT((uint32_t)type > 0);
    VkBufferUsageFlags usage = 0;
    switch (type)
    {
    case DVZ_BUFFER_TYPE_STAGING:
        usage = TRANSFERABLE;
        break;

    case DVZ_BUFFER_TYPE_VERTEX:
        usage = TRANSFERABLE |                      //
                VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | //
                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
        break;

    case DVZ_BUFFER_TYPE_INDEX:
        usage = TRANSFERABLE | //
                VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
        break;

    case DVZ_BUFFER_TYPE_STORAGE:
        usage = TRANSFERABLE | //
                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
        break;

    case DVZ_BUFFER_TYPE_UNIFORM:
        usage = TRANSFERABLE | //
                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        break;

    case DVZ_BUFFER_TYPE_INDIRECT:
        usage = TRANSFERABLE | //
                VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;
        break;

    default:
        log_error("could not find buffer usage for buffer type %d", type);
        break;
    }
    return usage;
}



static DvzBuffer* _make_new_buffer(DvzResources* res)
{
    ANN(res);
    DvzBuffer* buffer = (DvzBuffer*)dvz_container_alloc(&res->buffers);
    *buffer = dvz_buffer(res->gpu);
    ANN(buffer);

    // All buffers may be accessed from these queues.
    dvz_buffer_queue_access(buffer, DVZ_DEFAULT_QUEUE_TRANSFER);
    dvz_buffer_queue_access(buffer, DVZ_DEFAULT_QUEUE_COMPUTE);
    dvz_buffer_queue_access(buffer, DVZ_DEFAULT_QUEUE_RENDER);

    return buffer;
}



// NOT for staging
static void _make_shared_buffer(DvzBuffer* buffer, DvzBufferType type, bool mappable, DvzSize size)
{
    ANN(buffer);
    CHECK_BUFFER_TYPE
    ASSERT(type != DVZ_BUFFER_TYPE_STAGING);
    buffer->mappable_intended = mappable;

    dvz_buffer_size(buffer, size);
    VkBufferUsageFlags usage = _find_buffer_usage(type);
    ASSERT(usage != 0);
    dvz_buffer_usage(buffer, usage);
    dvz_buffer_type(buffer, type);
    dvz_buffer_vma_usage(
        buffer, mappable ? VMA_MEMORY_USAGE_CPU_TO_GPU : VMA_MEMORY_USAGE_GPU_ONLY);
    dvz_buffer_create(buffer);
    ASSERT(dvz_obj_is_created(&buffer->obj));
}



static void _make_staging_buffer(DvzBuffer* buffer, DvzSize size)
{
    ANN(buffer);
    dvz_buffer_type(buffer, DVZ_BUFFER_TYPE_STAGING);
    dvz_buffer_size(buffer, size);
    dvz_buffer_usage(buffer, TRANSFERABLE);
    dvz_buffer_vma_usage(buffer, VMA_MEMORY_USAGE_CPU_ONLY);
    dvz_buffer_create(buffer);
    ASSERT(dvz_obj_is_created(&buffer->obj));
}



static DvzBuffer*
_make_standalone_buffer(DvzResources* res, DvzBufferType type, bool mappable, DvzSize size)
{
    ANN(res);
    ASSERT((uint32_t)type > 0);
    ASSERT(size > 0);
    DvzBuffer* buffer = _make_new_buffer(res);
    buffer->mappable_intended = mappable;
    if (type == DVZ_BUFFER_TYPE_STAGING)
    {
        ASSERT(mappable);
        log_debug("create new staging buffer mappable %d size %s", mappable, pretty_size(size));
        _make_staging_buffer(buffer, size);
    }
    else
    {
        log_debug(
            "create new buffer with type %d (mappable: %d) with size %s", //
            type, mappable, pretty_size(size));
        _make_shared_buffer(buffer, type, mappable, size);
    }
    return buffer;
}



static DvzBuffer* _find_shared_buffer(DvzResources* res, DvzBufferType type, bool mappable)
{
    ANN(res);
    CHECK_BUFFER_TYPE

    DvzContainerIterator iter = dvz_container_iterator(&res->buffers);
    DvzBuffer* buffer = NULL;
    while (iter.item != NULL)
    {
        buffer = (DvzBuffer*)iter.item;
        ANN(buffer);
        // log_trace(
        //     "buffer %d=%d? %d=%d?", buffer->type, type, _is_buffer_mappable(buffer), mappable);
        if (dvz_obj_is_created(&buffer->obj) && buffer->type == type &&
            buffer->mappable_intended == mappable)
            return buffer;
        dvz_container_iter(&iter);
    }
    return NULL;
}



// Get an existing shared buffer, or create a new one if needed.
static DvzBuffer* _get_shared_buffer(DvzResources* res, DvzBufferType type, bool mappable)
{
    ANN(res);
    CHECK_BUFFER_TYPE

    DvzBuffer* buffer = _find_shared_buffer(res, type, mappable);
    if (buffer == NULL)
    {
        buffer = _make_standalone_buffer(res, type, mappable, DVZ_BUFFER_DEFAULT_SIZE);
        log_debug(
            "could not find shared buffer with type %d and mappable %d, so created a new one %d", //
            type, mappable, (uint64_t)buffer->buffer);
    }
    ANN(buffer);
    return buffer;
}



/*************************************************************************************************/
/*  Creation of buffer regions and images                                                        */
/*************************************************************************************************/

// Only for testing: bypass the resources system, useful for testing other modules
static DvzBufferRegions
_standalone_buffer_regions(DvzGpu* gpu, DvzBufferType type, uint32_t count, DvzSize size)
{
    ANN(gpu);
    ASSERT((uint32_t)type > 0);

    // Will be FREE-ed in _destroy_buffer_regions() below.
    DvzBuffer* buffer = (DvzBuffer*)calloc(1, sizeof(DvzBuffer));
    *buffer = dvz_buffer(gpu);
    if (type == DVZ_BUFFER_TYPE_STAGING)
        _make_staging_buffer(buffer, size * count);
    else if (type == DVZ_BUFFER_TYPE_VERTEX)
        _make_shared_buffer(buffer, DVZ_BUFFER_TYPE_VERTEX, true, size * count);
    DvzBufferRegions stg = dvz_buffer_regions(buffer, count, 0, size, 0);
    return stg;
}



static void _destroy_buffer_regions(DvzBufferRegions br)
{
    dvz_buffer_destroy(br.buffer);
    FREE(br.buffer);
}



static VkImageType _image_type_from_dims(DvzTexDims dims)
{
    switch (dims)
    {
    case DVZ_TEX_1D:
        return VK_IMAGE_TYPE_1D;
        break;
    case DVZ_TEX_2D:
        return VK_IMAGE_TYPE_2D;
        break;
    case DVZ_TEX_3D:
        return VK_IMAGE_TYPE_3D;
        break;

    default:
        break;
    }
    log_error("invalid image dimensions %d", dims);
    return VK_IMAGE_TYPE_2D;
}



static void _transition_image(DvzImages* img)
{
    ANN(img);
    DvzGpu* gpu = img->gpu;
    ANN(gpu);

    // DvzCommands cmds_ = dvz_commands(gpu, 0, 1);
    // DvzCommands* cmds = &cmds_;
    DvzCommands* cmds = &gpu->cmd;

    dvz_cmd_reset(cmds, 0);
    dvz_cmd_begin(cmds, 0);

    log_trace("starting image transition");
    VkAccessFlagBits2 access = VK_ACCESS_2_SHADER_SAMPLED_READ_BIT |
                               VK_ACCESS_2_SHADER_STORAGE_READ_BIT |
                               VK_ACCESS_2_SHADER_BINDING_TABLE_READ_BIT_KHR;
    DvzBarrier barrier = dvz_barrier(gpu);
    dvz_barrier_stages(&barrier, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
    dvz_barrier_images(&barrier, img);
    dvz_barrier_images_layout(&barrier, VK_IMAGE_LAYOUT_UNDEFINED, img->layout);
    dvz_barrier_images_access(&barrier, 0, access);
    dvz_cmd_barrier(cmds, 0, &barrier);

    dvz_cmd_end(cmds, 0);
    dvz_cmd_submit_sync(cmds, 0);
}



static void
_make_image(DvzGpu* gpu, DvzImages* img, DvzTexDims dims, uvec3 shape, DvzFormat format)
{
    ANN(img);
    log_trace("make images %dx%d%x", shape[0], shape[1], shape[2]);
    *img = dvz_images(gpu, _image_type_from_dims(dims), 1);

    // Create the image.
    dvz_images_format(img, (VkFormat)format);
    dvz_images_size(img, shape);
    dvz_images_tiling(img, VK_IMAGE_TILING_OPTIMAL);
    dvz_images_layout(img, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    dvz_images_usage(
        img,                                                      //
        VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | //
            VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);
    dvz_images_memory(img, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    dvz_images_queue_access(img, DVZ_DEFAULT_QUEUE_TRANSFER);
    dvz_images_queue_access(img, DVZ_DEFAULT_QUEUE_COMPUTE);
    dvz_images_queue_access(img, DVZ_DEFAULT_QUEUE_RENDER);
    dvz_images_create(img);

    // Immediately transition the image to its layout.
    _transition_image(img);
}



static DvzImages* _standalone_image(DvzGpu* gpu, DvzTexDims dims, uvec3 shape, DvzFormat format)
{
    ANN(gpu);
    ASSERT(1 <= dims && dims <= 3);
    log_debug(
        "creating %dD image with shape %dx%dx%d and format %d", //
        dims, shape[0], shape[1], shape[2], format);

    DvzImages* img = (DvzImages*)calloc(1, sizeof(DvzImages));
    _make_image(gpu, img, dims, shape, format);
    return img;
}



static void _destroy_image(DvzImages* img)
{
    ASSERT(img);
    dvz_images_destroy(img);
    FREE(img);
}



/*************************************************************************************************/
/*  Common resources                                                                             */
/*************************************************************************************************/

static void _create_resources(DvzResources* res)
{
    ANN(res);

    res->buffers = //
        dvz_container(DVZ_CONTAINER_DEFAULT_COUNT, sizeof(DvzBuffer), DVZ_OBJECT_TYPE_BUFFER);
    res->images = //
        dvz_container(DVZ_CONTAINER_DEFAULT_COUNT, sizeof(DvzImages), DVZ_OBJECT_TYPE_IMAGES);
    res->dats = //
        dvz_container(DVZ_CONTAINER_DEFAULT_COUNT, sizeof(DvzDat), DVZ_OBJECT_TYPE_DAT);
    res->texs = //
        dvz_container(DVZ_CONTAINER_DEFAULT_COUNT, sizeof(DvzTex), DVZ_OBJECT_TYPE_TEX);
    res->samplers = //
        dvz_container(DVZ_CONTAINER_DEFAULT_COUNT, sizeof(DvzSampler), DVZ_OBJECT_TYPE_SAMPLER);
}



static void _destroy_resources(DvzResources* res)
{
    ANN(res);

    log_trace("context destroy buffers");
    CONTAINER_DESTROY_ITEMS(DvzBuffer, res->buffers, dvz_buffer_destroy)

    log_trace("context destroy sets of images");
    CONTAINER_DESTROY_ITEMS(DvzImages, res->images, dvz_images_destroy)

    log_trace("context destroy texs");
    CONTAINER_DESTROY_ITEMS(DvzTex, res->texs, dvz_tex_destroy)

    log_trace("context destroy dats");
    CONTAINER_DESTROY_ITEMS(DvzDat, res->dats, dvz_dat_destroy)

    log_trace("context destroy samplers");
    CONTAINER_DESTROY_ITEMS(DvzSampler, res->samplers, dvz_sampler_destroy)
}



/*************************************************************************************************/
/*  Dat utils                                                                                    */
/*************************************************************************************************/

static inline bool _dat_is_standalone(DvzDat* dat)
{
    ANN(dat);
    return (dat->flags & DVZ_DAT_FLAGS_STANDALONE) != 0;
}



static inline bool _dat_has_staging(DvzDat* dat)
{
    ANN(dat);
    return (dat->flags & DVZ_DAT_FLAGS_MAPPABLE) == 0;

    // OLD NOTE: there might be a mismatch between DVZ_DAT_FLAGS_MAPPABLE (whether we intend
    // the dat to be mappable), and buffer->memory & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
    // (whether the underlyling buffer is actually mappable or not), for example with a software
    // rendered such as swiftshader where ALL buffers are mappable.
    // Therefore this function should really return whether the dat needs a staging buffer
    // (because the underlying buffer is not mappable) and not whether we intended the dat
    // to be mappable or not.
    // if (dat->br.buffer == NULL)
    //     return (dat->flags & DVZ_DAT_FLAGS_MAPPABLE) == 0;
    // else
    //     return !_is_buffer_mappable(dat->br.buffer);
}



static inline bool _dat_is_dup(DvzDat* dat)
{
    ANN(dat);
    return (dat->flags & DVZ_DAT_FLAGS_DUP) != 0;
}



static inline bool _dat_keep_on_resize(DvzDat* dat)
{
    ANN(dat);
    return (dat->flags & DVZ_DAT_FLAGS_KEEP_ON_RESIZE) != 0;
}



static inline bool _dat_persistent_staging(DvzDat* dat)
{
    ANN(dat);
    return (dat->flags & DVZ_DAT_FLAGS_PERSISTENT_STAGING) != 0;
}



static inline DvzSize
_total_aligned_size(DvzBuffer* buffer, uint32_t count, DvzSize size, DvzSize* alignment)
{
    // Find the buffer alignment.
    *alignment = buffer->vma.alignment;
    // Make sure the requested size is aligned.
    return count * _align(size, *alignment);
}



/*************************************************************************************************/
/*  Dat allocation                                                                               */
/*************************************************************************************************/

static inline bool _is_dat_valid(DvzDat* dat)
{
    ANN(dat);
    return dat->br.buffer != NULL && dat->br.buffer->buffer != VK_NULL_HANDLE;
}



static inline DvzDat* _alloc_staging(DvzContext* ctx, DvzSize size)
{
    ANN(ctx);
    return dvz_dat(ctx, DVZ_BUFFER_TYPE_STAGING, size, 0);
}



static void
_dat_alloc(DvzResources* res, DvzDat* dat, DvzBufferType type, uint32_t count, DvzSize size)
{
    ANN(res);
    ANN(dat);

    DvzBuffer* buffer = NULL;
    DvzSize offset = 0; // to determine with allocator if shared buffer
    DvzSize alignment = 0;
    DvzSize tot_size = 0;

    bool shared = !_dat_is_standalone(dat);
    bool mappable = !_dat_has_staging(dat);

    // Shared buffer.
    if (shared)
    {
        // Get the unique shared buffer of the requested type.
        buffer = _get_shared_buffer(res, type, mappable);

        // Find the buffer alignment and total aligned size.
        tot_size = _total_aligned_size(buffer, count, size, &alignment);

        // Allocate a DvzDat from it.
        // NOTE: this call may resize the underlying DvzBuffer, which is slow (hard GPU sync).
        offset = dvz_datalloc_alloc(dat->datalloc, res, type, mappable, tot_size);
    }

    // Standalone buffer.
    else
    {
        // Create a brand new buffer just for this DvzDat.
        buffer = dvz_resources_buffer(res, type, mappable, count * size);
        // Allocate the entire buffer, so offset is 0, and the size is the requested (aligned if
        // necessary) size.
        offset = 0;
        // NOTE: for standalone buffers, we should not need to worry about alignments at this point
    }

    // Check alignment.
    if (alignment > 0)
        ASSERT(offset % alignment == 0);

    if (offset + tot_size > buffer->size)
    {
        log_error(
            "buffer %d too small %d %d %d", //
            (uint64_t)buffer->buffer, offset, tot_size, buffer->size);
        return;
    }
    if (buffer->buffer == VK_NULL_HANDLE)
    {
        log_error("dat allocation failed");
        return;
    }

    log_debug(
        "allocate dat, buffer type %d, flags %d, offset %d, %s%ssize %s", //
        type, dat->flags, offset,                                         //
        shared ? "shared, " : "standalone, ",                             //
        mappable ? "mappable, " : "unmappable, ",                         //
        pretty_size(size));

    // Set the buffer region.
    dat->br = dvz_buffer_regions(buffer, count, offset, size, alignment);
}



static void _dat_dealloc(DvzDat* dat)
{
    ANN(dat);

    if (dat->br.buffer == NULL)
    {
        return;
    }

    log_debug(
        "deallocate dat %u, offset %d, size %s", //
        dat, dat->br.offsets[0], pretty_size(dat->br.size));

    bool shared = !_dat_is_standalone(dat);
    bool mappable = !_dat_has_staging(dat);

    if (shared)
    {
        // Deallocate the buffer regions but keep the underlying buffer.
        dvz_datalloc_dealloc(dat->datalloc, dat->br.buffer->type, mappable, dat->br.offsets[0]);
    }
    else
    {
        // Destroy the standalone buffer.
        dvz_buffer_destroy(dat->br.buffer);
    }
}



/*************************************************************************************************/
/*  Tex utils                                                                                    */
/*************************************************************************************************/

static inline bool _tex_persistent_staging(DvzTex* tex)
{
    ANN(tex);
    return (tex->flags & DVZ_TEX_FLAGS_PERSISTENT_STAGING) != 0;
}



static DvzDat* _tex_staging(DvzContext* ctx, DvzTex* tex, DvzSize size)
{
    ANN(ctx);
    ANN(tex);
    DvzDat* stg = tex->stg;
    if (stg != NULL)
        return stg;

    // Need to allocate a staging buffer.
    log_debug("allocate staging buffer with size %s for tex", pretty_size(size));
    stg = _alloc_staging(ctx, size);

    // If persistent staging, store it.
    if (_tex_persistent_staging(tex))
    {
        tex->stg = stg;
    }

    return stg;
}



static void
_tex_alloc(DvzResources* res, DvzTex* tex, DvzTexDims dims, DvzFormat format, uvec3 shape)
{
    ANN(res);
    ANN(tex);

    // Create a new image for the tex.
    tex->img = dvz_resources_image(res, dims, shape, format);
}



static inline DvzSize _format_size(DvzFormat format)
{
    switch (format)
    {
    case DVZ_FORMAT_R8_UNORM:
    case DVZ_FORMAT_R8_SNORM:
    case DVZ_FORMAT_R8_UINT:
    case DVZ_FORMAT_R8_SINT:
        return 1 * 1;
        break;

    case DVZ_FORMAT_R8G8_UNORM:
    case DVZ_FORMAT_R8G8_SNORM:
    case DVZ_FORMAT_R8G8_UINT:
    case DVZ_FORMAT_R8G8_SINT:
        return 2 * 1;
        break;

    case DVZ_FORMAT_R16_UNORM:
    case DVZ_FORMAT_R16_SNORM:
        return 1 * 2;
        break;

    case DVZ_FORMAT_R32_UINT:
    case DVZ_FORMAT_R32_SINT:
    case DVZ_FORMAT_R32_SFLOAT:
        return 1 * 4;
        break;

    case DVZ_FORMAT_R8G8B8_UNORM:
    case DVZ_FORMAT_R8G8B8_SNORM:
    case DVZ_FORMAT_R8G8B8_UINT:
    case DVZ_FORMAT_R8G8B8_SINT:
        return 3 * 1;
        break;

    case DVZ_FORMAT_R8G8B8A8_UNORM:
    case DVZ_FORMAT_R8G8B8A8_SNORM:
    case DVZ_FORMAT_R8G8B8A8_UINT:
    case DVZ_FORMAT_R8G8B8A8_SINT:
    case DVZ_FORMAT_B8G8R8A8_UNORM:
        return 4 * 1;
        break;

    case DVZ_FORMAT_R32G32_UINT:
    case DVZ_FORMAT_R32G32_SINT:
    case DVZ_FORMAT_R32G32_SFLOAT:
        return 2 * 4;
        break;

    case DVZ_FORMAT_R32G32B32_UINT:
    case DVZ_FORMAT_R32G32B32_SINT:
    case DVZ_FORMAT_R32G32B32_SFLOAT:
        return 3 * 4;
        break;

    case DVZ_FORMAT_R32G32B32A32_UINT:
    case DVZ_FORMAT_R32G32B32A32_SINT:
    case DVZ_FORMAT_R32G32B32A32_SFLOAT:
        return 4 * 4;
        break;

    default:
        break;
    }
    log_error("unknown DvzFormat %d", format);
    return 0;
}



static inline DvzSize _tex_size(DvzFormat format, uvec3 shape)
{
    DvzSize size = _format_size(format) * shape[0] * shape[1] * shape[2];
    ASSERT(size > 0);
    return size;
}



static void _tex_dealloc(DvzTex* tex)
{
    ANN(tex);

    // Destroy the image.
    dvz_images_destroy(tex->img);
}



#endif
